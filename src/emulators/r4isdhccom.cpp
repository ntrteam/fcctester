#include <string>
#include <iostream>
#include <cstring>

#include "../emulator.h"

class R4isdhcComEmu : public Emulator {
public:
    R4isdhcComEmu() :
        Emulator("R4iSDHC family", 0x200000, 0xFC2),
        _writeInProgress(false),
        _writeEnabled(false),
        _aapPassed(false),
        _writeArg(-1),
        _writeCounter(-1) {}

    virtual void reset() override {}

    virtual bool ntrReadCommand(std::uint64_t cmd, void *dest, std::size_t dest_size, ncgc::NTRFlags flags) override {
        if (_card->state() != ncgc::NTRState::Key2 && !_aapPassed) {
            return false;
        }

        if ((cmd & 0xFF) == 0x66) {
            _aapPassed = true;
            if (dest) {
                std::memset(dest, 0, dest_size);
            }
            return true;
        } else if (!_aapPassed) {
            return false;
        } else if ((cmd & 0xFF) == 0x99) {
            if (dest) {
                std::memset(dest, 0, dest_size);
            }
            if (_writeInProgress) {
                switch ((cmd & 0xFF00) >> 8) {
                case 0xF0:
                    _writeInProgress = false;
                    _writeCounter = _writeArg = -1;
                    break;
                case 0:
                    writeData((cmd & 0xFF0000ull) >> 16, (cmd & 0xFF000000ull) >> 24);
                    break;
                default:
                    return false;
                }
            } else {
                switch ((cmd & 0xFF0000ull) >> 16) {
                case 6:
                    _writeEnabled = true;
                    break;
                case 4:
                    _writeEnabled = false;
                    break;
                case 0x20: {
                    if (_writeEnabled) {
                        std::uint32_t addr = extractAddress(cmd);
                        std::memset(_flash + (addr & 0x1FF000), 0xFF, 0x1000);
                        _writeEnabled = false;
                    }
                    break;
                }
                case 2: {
                    if (_writeEnabled) {
                        std::uint32_t addr = extractAddress(cmd);
                        _writeInProgress = true;
                        _writeArg = addr;
                        _writeCounter = 0;
                        _writeEnabled = false;
                        writeData((cmd & 0xFF000000000000ull) >> 48, (cmd & 0xFF00000000000000ull) >> 56);
                    }
                    break;
                }
                case 0x3B: {
                    std::uint32_t addr = extractAddress(cmd);
                    std::uint8_t *destu8 = static_cast<std::uint8_t *>(dest);
                    destu8[0] = _flash[addr++ & 0x1FFFFF];
                    destu8[1] = _flash[addr++ & 0x1FFFFF];
                    destu8[2] = _flash[addr++ & 0x1FFFFF];
                    destu8[3] = _flash[addr++ & 0x1FFFFF];
                    break;
                }
                default:
                    return false;
                }
            }
            return true;
        }

        return false;
    }

    virtual void ntrWriteCommand(std::uint64_t cmd, const void *src, std::size_t src_size, ncgc::NTRFlags flags) override {}

    virtual std::uint8_t spiTransact(std::uint8_t in, bool last) override {
        return 0;
    }
private:
    bool _writeInProgress;
    bool _writeEnabled;
    bool _aapPassed;
    std::int32_t _writeArg;
    std::int32_t _writeCounter;

    void writeData(std::uint8_t b1, std::uint8_t b2) {
        const std::int32_t addr = (_writeArg & 0x1FFF00) | (_writeCounter & 0xFF);
        _flash[addr] = b1;
        _flash[addr + 1] = b2;
        _writeCounter += 2;
    }

    static constexpr std::uint32_t extractAddress(const std::uint64_t cmd) {
        // ?? ?? ?? AA AA AA ?? ?? 99
        return
            ((cmd & 0xFF000000ull) >> 8) |
            ((cmd & 0xFF00000000ull) >> 24) |
            ((cmd & 0xFF0000000000ull) >> 40);
    }
};

namespace {
R4isdhcComEmu _;
}
