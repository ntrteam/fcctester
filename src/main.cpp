#include <algorithm>
#include <string>
#include <iostream>
#include <cstdint>

#include "flashcart_core/device.h"
#include "flashcart_core/platform.h"

#include "emulator.h"

namespace {
std::string stringToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
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

    return 0;
}
