#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <cstdint>

#include "flashcart_core/device.h"
#include "flashcart_core/platform.h"

#include "emulator.h"

namespace {
std::string stringToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::random_device rand;

void memrand(std::uint8_t *buf, std::size_t sz) {
    for (std::uint32_t i = 0; i < sz; ++i) {
        buf[i] = rand();
    }
}

uint32_t randu32(std::uint32_t lo, std::uint32_t hi) {
    return std::uniform_int_distribution<uint32_t>(lo, hi)(rand);

}

bool readTest(flashcart_core::Flashcart *fc, Emulator *emu, std::uint32_t addr, std::uint32_t size) {
    std::uint8_t *const flash = emu->flashContents();
    std::uint8_t *const buf = new std::uint8_t[size];
    memrand(flash + addr, size);

    std::cout << "Read from " << addr << " of size " << size << std::endl;

    if (!fc->readFlash(addr, size, buf)) {
        std::cout << "Flashcart read failed." << std::endl;
        delete[] buf;
        return false;
    }

    if (std::memcmp(buf, flash + addr, size)) {
        std::cout << "Read result wrong" << std::endl;
        delete[] buf;
        return false;
    }

    delete[] buf;
    return true;
}

bool writeTest(flashcart_core::Flashcart *fc, Emulator *emu, std::uint32_t addr, std::uint32_t size) {
    std::uint8_t *const flash = emu->flashContents();
    const std::uint32_t flashSz = fc->getMaxLength();
    std::uint8_t *const old_flash = new std::uint8_t[flashSz];
    std::uint8_t *const buf = new std::uint8_t[size];
    std::memcpy(old_flash, flash, flashSz);
    memrand(buf, size);
    std::memcpy(old_flash + addr, buf, size);

    std::cout << "Write to " << addr << " of size " << size << std::endl;

    bool success = true;
    
    if (!fc->writeFlash(addr, size, buf)) {
        std::cout << "Flashcart write failed." << std::endl;
        success = false;
        goto end;
    }

    if (std::memcmp(buf, flash + addr, size)) {
        std::cout << "Write result wrong" << std::endl;
        success = false;
        goto end;
    }
    
    if (std::memcmp(old_flash, flash, flashSz)) {
        std::cout << "Write seems to have destroyed data outside of written area" << std::endl;
        success = false;
        goto end;
    }

end:
    delete[] old_flash;
    delete[] buf;
    return success;
}
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " " << "<flashcart name (substring acceptable)> [emulator name = flashcart name]" << std::endl;
        return 1;
    }

    flashcart_core::Flashcart *fc = nullptr;
    for (flashcart_core::Flashcart *&tfc : *flashcart_core::flashcart_list) {
        // i'm sure we're not going to have i18n issues with flashcart names, right??!
        if (stringToLower(std::string(tfc->getName())).find(stringToLower(argv[1])) != std::string::npos) {
            fc = tfc;
            break;
        }
    }
    if (!fc) {
        std::cout << "No flashcart matched \"" << argv[1] << "\"." << std::endl;
        return 1;
    }

    const char *emuName = argc >= 3 ? argv[2] : fc->getName();
    Emulator *emu = nullptr;
    for (Emulator *&temu : *Emulator::list) {
        if (stringToLower(std::string(temu->name())).find(stringToLower(emuName)) != std::string::npos) {
            emu = temu;
            break;
        }
    }
    if (!emu) {
        std::cout << "No emulator matched \"" << emuName << "\"." << std::endl;
        return 1;
    }

    std::cout << "Testing flashcart_core class " << fc->getName();
    std::cout << " with emulator " << emu->name() << std::endl;

    emu->init();
    if (!fc->initialize(emu->ntrCard())) {
        std::cout << "Flashcart init failed." << std::endl;
        return 1;
    }
    const std::uint32_t fsz = fc->getMaxLength();

    if (!readTest(fc, emu, 0, fc->getMaxLength())) {
        return 1;
    }

    if (!writeTest(fc, emu, 0, fc->getMaxLength())) {
        return 1;
    }

    for (int i = 0; i < 0x10; ++i) {
        std::uint32_t randAddr = randu32(0, fsz >> 1) | 1;
        std::uint32_t randSz = randu32(0, fsz - randAddr - 2) | 1;
        if (!readTest(fc, emu, randAddr, randSz)) {
            return 1;
        }
    }

    for (int i = 0; i < 0x10; ++i) {
        std::uint32_t randAddr = randu32(0, fsz >> 1) | 1;
        std::uint32_t randSz = randu32(0, fsz - randAddr - 2) | 1;
        if (!writeTest(fc, emu, randAddr, randSz)) {
            return 1;
        }
    }

    for (int i = 0; i < 0x10; ++i) {
        std::uint32_t randAddr = randu32(0, fsz >> 1) | 1;
        std::uint32_t randSz = randu32(0, 0x10) | 1;
        if (!readTest(fc, emu, randAddr, randSz)) {
            return 1;
        }
    }

    for (int i = 0; i < 0x10; ++i) {
        std::uint32_t randAddr = randu32(0, fsz >> 1) | 1;
        std::uint32_t randSz = randu32(0, 0x10) | 1;
        if (!writeTest(fc, emu, randAddr, randSz)) {
            return 1;
        }
    }

    for (int i = 0; i < 0x10; ++i) {
        std::uint32_t firmsz = randu32(0x10000, 0x20000) & ~0xFF;
        std::uint8_t *buf = new uint8_t[firmsz + 0x1048];
        memrand(buf, firmsz + 0x1048);

        if (!fc->injectNtrBoot(buf, buf + 0x1048, firmsz)) {
            std::cout << "NTRboot inject failed" << std::endl;
        }
        delete[] buf;
    }

    std::cout << "All OK." << std::endl;
    return 0;
}
