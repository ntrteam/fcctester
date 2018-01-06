#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>

#include <ncgcpp/ntrcard.h>

#include "emulator.h"

namespace {
constexpr Emulator *ncgcCardToEmulator(ncgc::c::ncgc_ncard_t *card) {
    return static_cast<Emulator *>(card->platform.data.ptr_data);
}

void platformIoDelay(std::uint32_t delay) {
    static_cast<void>(delay);
}

void platformSeedKey2(ncgc::c::ncgc_ncard_t *card, std::uint64_t x, std::uint64_t y) {
    static_cast<void>(card); static_cast<void>(x); static_cast<void>(y);
}

ncgc::c::ncgc_err_t platformReset(ncgc::c::ncgc_ncard_t *card) {
    Emulator *e = ncgcCardToEmulator(card);
    e->reset();
    return ncgc::c::NCGC_EOK;
}

ncgc::c::ncgc_err_t platformSendReadCommand(ncgc::c::ncgc_ncard_t *card, uint64_t cmd, uint32_t readsz, void *buf, uint32_t bufsz, ncgc::c::ncgc_nflags_t flags) {
    Emulator *e = ncgcCardToEmulator(card);
    if (bufsz < readsz || !buf) {
        std::uint8_t *fullBuf = new uint8_t[readsz];
        e->ntrReadCommand(cmd, fullBuf, readsz, flags);
        if (buf && bufsz) {
            memcpy(buf, fullBuf, bufsz);
        }
        delete[] fullBuf;
    } else {
        e->ntrReadCommand(cmd, buf, readsz, flags);
    }
    return ncgc::c::NCGC_EOK;
}

ncgc::c::ncgc_err_t platformSendWriteCommand(ncgc::c::ncgc_ncard_t *card, uint64_t cmd, const void *buf, uint32_t bufsz, ncgc::c::ncgc_nflags_t flags) {
    Emulator *e = ncgcCardToEmulator(card);
    e->ntrWriteCommand(cmd, buf, bufsz, flags);
    return ncgc::c::NCGC_EOK;
}

ncgc::c::ncgc_err_t platformSpiTransact(ncgc::c::ncgc_ncard_t *card, std::uint8_t bin, std::uint8_t *bout, bool last) {
    Emulator *e = ncgcCardToEmulator(card);
    *bout = e->spiTransact(bin, last);
    return ncgc::c::NCGC_EOK;
}
}

std::vector<Emulator *> *Emulator::list = new std::vector<Emulator *>();

Emulator::Emulator(std::string name, std::size_t flash_size, std::uint32_t chipid) :
        _name(name),
        _card(nullptr),
        _flash(nullptr),
        _flash_size(flash_size),
        _chipid(chipid) {
    list->push_back(this);
}

Emulator::~Emulator() {
    delete _card;
    delete[] _flash;
}

void Emulator::init() {
    if (!_card) {
        _card = new ncgc::NTRCard();
        ncgc::c::ncgc_nplatform_t& rpl = _card->rawState().platform;
        rpl.io_delay = platformIoDelay;
        rpl.reset = ::platformReset;
        rpl.seed_key2 = platformSeedKey2;
        rpl.send_command = platformSendReadCommand;
        rpl.send_write_command = platformSendWriteCommand;
        rpl.spi_transact = platformSpiTransact;
        rpl.hw_key2 = true;
        rpl.ignore_key1 = true;
        rpl.data.ptr_data = this;
    }

    if (!_flash) {
        _flash = new uint8_t[_flash_size];
    }
}

void Emulator::platformReset() {
    _card->state(ncgc::NTRState::Raw);
}

void Emulator::ntrPlatformReadCommand(std::uint64_t cmd, void *dest, std::size_t dest_size, ncgc::NTRFlags flags) {
    if (ntrReadCommand(cmd, dest, dest_size, flags)) {
        return;
    }

    switch (_card->state()) {
    case ncgc::NTRState::Raw:
        switch (cmd & 0xFF) {
        case 0x9F:
        case 0x00:
            // response unused by ncgc
            break;
        case 0x90:
            std::memcpy(dest, &_chipid, std::min<std::size_t>(dest_size, 4));
            break;
        }
        break;
    case ncgc::NTRState::Key1:
        switch (cmd & 0xF0) {
        case 0x10:
            std::memcpy(dest, &_chipid, std::min<std::size_t>(dest_size, 4));
            break;
        }
        break;
    case ncgc::NTRState::Key2:
        switch (cmd & 0xFF) {
        case 0xB8:
            std::memcpy(dest, &_chipid, std::min<std::size_t>(dest_size, 4));
            break;
        }
        break;
    default:
        std::cout << "Invalid state encountered in Emulator::ntrPlatformReadCommand" << std::endl;
        break;
    }

}

std::string& Emulator::name() {
    return _name;
}

ncgc::NTRCard *Emulator::ntrCard() {
    return _card;
}

std::uint8_t *Emulator::flashContents() {
    return _flash;
}

std::size_t Emulator::flashSize() {
    return _flash_size;
}
