// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <cwctype>

#include <windows.h>
#include <bcrypt.h>

namespace fusion {
namespace debugviewpp {

// Where save_log / save_view write. The AI only ever supplies a file name.
enum class McpExportMode : int
{
    Documents = 0,  // Documents\UltimateDebugView
    Folder = 1,     // a folder the user chose
    AppFolder = 2,  // the folder of the exe that wrote the log lines
};

struct McpSettings
{
    bool enabled = false;
    int port = 7777;
    std::wstring token;  // required while enabled; generated automatically when missing
    bool maskSecrets = true;
    McpExportMode exportMode = McpExportMode::Documents;
    std::wstring exportFolder;     // used by McpExportMode::Folder
    std::wstring exportSubfolder;  // optional plain folder name added under the chosen folder, e.g. DebugLogs

    bool operator==(const McpSettings& other) const
    {
        return enabled == other.enabled && port == other.port && token == other.token && maskSecrets == other.maskSecrets &&
               exportMode == other.exportMode && exportFolder == other.exportFolder && exportSubfolder == other.exportSubfolder;
    }
    bool operator!=(const McpSettings& other) const { return !(*this == other); }
};

constexpr size_t McpMinTokenLength = 16;

// A plain file or folder name: letters, digits, space, . _ - ; no leading dot, no trailing dot/space, max 120.
inline bool IsPlainMcpName(const std::wstring& name)
{
    if (name.empty() || name.size() > 120 || name.front() == L'.' || name.back() == L'.' || name.back() == L' ')
        return false;
    for (wchar_t c : name)
    {
        if (::iswalnum(c) == 0 && c != L'.' && c != L'_' && c != L'-' && c != L' ')
            return false;
    }
    return true;
}

// 48 hex characters from the system CSPRNG; empty on failure.
inline std::wstring GenerateMcpToken()
{
    unsigned char bytes[24] = {};
    if (!BCRYPT_SUCCESS(::BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
        return {};
    static const wchar_t hex[] = L"0123456789abcdef";
    std::wstring token;
    for (unsigned char b : bytes)
    {
        token += hex[b >> 4];
        token += hex[b & 0x0F];
    }
    return token;
}

} // namespace debugviewpp
} // namespace fusion
