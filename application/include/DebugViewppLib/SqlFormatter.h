#pragma once

#include <string>

namespace fusion {
namespace debugviewpp {

// Format SQL statement with proper indentation and line breaks
std::wstring FormatSQL(const std::wstring& sql);

} // namespace debugviewpp
} // namespace fusion