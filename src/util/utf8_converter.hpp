#pragma once

#include <string>

namespace utf8 {
    std::string toLatin1(const std::string& utf8_string);
    std::string toUtf8(const std::string& latin1_string);
}
