#include <string>
#include <vector>
#include <cstdint>

#include <ncgcpp/ntrcard.h>

class Emulator {
public:
    virtual ~Emulator();

    virtual void init();
    virtual void platformReset();
    virtual void ntrPlatformReadCommand(std::uint64_t cmd, void *dest, std::size_t dest_size, ncgc::NTRFlags flags);

    virtual void reset() = 0;
    virtual bool ntrReadCommand(std::uint64_t cmd, void *dest, std::size_t dest_size, ncgc::NTRFlags flags) = 0;
    virtual void ntrWriteCommand(std::uint64_t cmd, const void *src, std::size_t src_size, ncgc::NTRFlags flags) = 0;
    virtual std::uint8_t spiTransact(std::uint8_t in, bool last) = 0;

    virtual ncgc::NTRCard *ntrCard();
    virtual std::string &name();
    virtual std::size_t flashSize();
    virtual std::uint8_t *flashContents();

    static std::vector<Emulator *> *list;

protected:
    Emulator(std::string name, std::size_t flash_size, std::uint32_t chipid = 0xFC2);

    std::string _name;
    ncgc::NTRCard *_card;
    std::uint8_t *_flash;
    std::size_t _flash_size;
    std::uint32_t _chipid;
};
