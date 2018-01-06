#include <string>

#include "../emulator.h"

class Ace3DSEmu : public Emulator {
public:
    Ace3DSEmu() : Emulator("Ace3DS+", 0x200000, 0xFC2) {}

    virtual void reset() override {
    }

    virtual bool ntrReadCommand(std::uint64_t cmd, void *dest, std::size_t dest_size, ncgc::NTRFlags flags) override {
        return false;
    }

    virtual void ntrWriteCommand(std::uint64_t cmd, const void *src, std::size_t src_size, ncgc::NTRFlags flags) override {
    }

    virtual std::uint8_t spiTransact(std::uint8_t in, bool last) override {
        return 0;
    }
};

namespace {
Ace3DSEmu _;
}
