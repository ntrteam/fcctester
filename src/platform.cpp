#include <iostream>
#include <string>
#include <random>
#include <cstdarg>
#include <cstdio>

#include "flashcart_core/platform.h"

std::uint8_t blowfish_key[0x1048] = { 0 };

std::string priorityString(flashcart_core::log_priority priority) {
    switch (priority) {
    case flashcart_core::LOG_DEBUG:
        return "DEBUG";
    case flashcart_core::LOG_INFO:
        return "INFO";
    case flashcart_core::LOG_NOTICE:
        return "NOTICE";
    case flashcart_core::LOG_WARN:
        return "WARN";
    case flashcart_core::LOG_ERR:
        return "ERR";
    default:
        return std::to_string(static_cast<int>(priority));
    }
}

namespace flashcart_core {
namespace platform {
void showProgress(std::uint32_t current, std::uint32_t total, const char* status_string) {
    static const char *last_status = nullptr;
    static int last_percent = 0xFAFAFAFAu;

    const int cur_percent = current*100/total;
    if (last_status != status_string || std::abs(last_percent - cur_percent) > 10 || cur_percent >= 100) {
        std::cout << "Progress: " << status_string << " " << cur_percent << "%" << std::endl;
        last_status = status_string;
        last_percent = cur_percent;
    }
}

int logMessage(log_priority priority, const char *fmt, ...) {
    // TODO FIXME add command line option for log level
    if (priority < LOG_INFO) { return 0; }
    std::cout << "flashcart_core [" << priorityString(priority) << "] ";
    std::va_list args;
    va_start(args, fmt);
    int ret = std::vprintf(fmt, args);
    va_end(args);
    std::cout << std::endl;
    return ret;
}

auto getBlowfishKey(BlowfishKey key) -> const std::uint8_t(&)[0x1048] {
    static_cast<void>(key);
    return blowfish_key;
}
}
}
