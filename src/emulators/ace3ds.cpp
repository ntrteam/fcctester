#include <string>
#include <cstring>

#include "../emulator.h"

class Ace3DSEmu : public Emulator {
public:
    Ace3DSEmu() :
        Emulator("Ace3DS+", 0x200000, 0xFC2),
        _spiInProgress(false),
        _writeEnabled(false),
        _spiCommand(0xDEADDEAD),
        _spiArg(0),
        _spiCounter(0xDEADDEAD) {}

    virtual void init() override {
        Emulator::init();
        // skip the whole init dance
        _card->state(ncgc::NTRState::Key2);
    }

    virtual void reset() override {
    }

    virtual bool ntrReadCommand(std::uint64_t cmd, void *dest, std::size_t dest_size, ncgc::NTRFlags flags) override {
        if (_card->state() != ncgc::NTRState::Key2) {
            return false;
        }

        switch (cmd & 0xFF) {
        case 0xB0: {
            std::uint32_t resp = 0x22A00000;
            std::memcpy(dest, &resp, 4);
            return true;
        }
        }

        return false;
    }

    virtual void ntrWriteCommand(std::uint64_t cmd, const void *src, std::size_t src_size, ncgc::NTRFlags flags) override {
    }

    virtual std::uint8_t spiTransact(std::uint8_t in, bool last) override {
        if (!_spiInProgress) {
            switch (in) {
            case 6: // SPI WREN
                _writeEnabled = true;
                break;
            default:
                _spiCommand = in;
                _spiCounter = 0;
                _spiArg = 0;
                _spiInProgress = true;
                break;
            }
            return 0xFF;
        } else {
            std::uint8_t ret = 0xFF;
            switch (_spiCommand) {
            case 0x9F: // SPI RDID
                static uint8_t rdid[] = { 0xC8, 0x40, 0x15 };
                if (_spiCounter <= 2) {
                    ret = rdid[_spiCounter];
                }
                break;
            case 3: // SPI Read
                if (!spiLatchArg(in)) {
                    if (_spiArg + _spiCounter - 3 < _flash_size) {
                        ret = _flash[_spiArg + _spiCounter - 3];
                    }
                }
                break;
            case 2: // SPI Write
                if (!spiLatchArg(in)) {
                    std::size_t addr = (_spiArg & 0xFFFF00) | ((_spiArg + _spiCounter - 3) & 0xFF);
                    if (addr < _flash_size && _writeEnabled) {
                        _flash[addr] &= in;
                    }
                }
                if (last) {
                    _writeEnabled = false;
                }
                break;
            case 0x20: // SPI sector erase
                spiLatchArg(in);
                _spiArg &= 0xFFF000;
                if (_spiCounter == 2 && _spiArg < _flash_size && _writeEnabled) {
                    std::memset(_flash + _spiArg, 0xFF, std::min<std::size_t>(0x1000, _flash_size - _spiArg));
                    _writeEnabled = false;
                }
                break;
            case 5:
                ret = 0;
                break;
            }
            if (last) {
                _spiCommand = 0xDEADDEAD;
                _spiCounter = 0xDEADDEAD;
                _spiInProgress = false;
            } else {
                _spiCounter++;
            }
            return ret;
        }
    }
private:
    bool _spiInProgress;
    bool _writeEnabled;
    int _spiCommand;
    std::size_t _spiArg;
    int _spiCounter;

    bool spiLatchArg(std::uint8_t in) {
        if (_spiCounter > 2) {
            return false;
        }

        _spiArg |= in << (8 * (2 - _spiCounter));
        return true;
    }
};

namespace {
Ace3DSEmu _;
}
