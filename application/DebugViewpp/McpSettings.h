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
//   <base>\<sub-folder>[\<app name>]\<file>
// base = Documents or a folder the user chose; in Documents an empty sub-folder means "UltimateDebugView".
enum class McpExportMode : int
{
    Documents = 0,
    Folder = 1,
};

struct McpSettings
{
    bool enabled = false;
    int port = 7777;
    bool requireToken = false;  // only processes of the same Windows user may connect; the token is an extra
    std::wstring token;         // used when requireToken; generated automatically when missing
    bool maskSecrets = true;
    McpExportMode exportMode = McpExportMode::Documents;
    std::wstring exportFolder;     // used by McpExportMode::Folder
    std::wstring exportSubfolder;  // optional plain folder name, e.g. DebugLogs
    bool exportPerApp = false;     // add a folder named after the exe, e.g. PFBExec

    bool operator==(const McpSettings& other) const
    {
        return enabled == other.enabled && port == other.port && requireToken == other.requireToken && token == other.token && maskSecrets == other.maskSecrets &&
               exportMode == other.exportMode && exportFolder == other.exportFolder && exportSubfolder == other.exportSubfolder &&
               exportPerApp == other.exportPerApp;
    }
    bool operator!=(const McpSettings& other) const { return !(*this == other); }
};

constexpr size_t McpMinTokenLength = 16;

// CON, PRN, AUX, NUL, COM1-9, LPT1-9 (with or without an extension) name devices, not files.
inline bool IsReservedWindowsName(const std::wstring& name)
{
    std::wstring stem = name.substr(0, name.find(L'.'));
    while (!stem.empty() && stem.back() == L' ')
        stem.pop_back();
    for (auto& c : stem)
        c = static_cast<wchar_t>(::towupper(c));
    if (stem == L"CON" || stem == L"PRN" || stem == L"AUX" || stem == L"NUL")
        return true;
    return stem.size() == 4 && (stem.compare(0, 3, L"COM") == 0 || stem.compare(0, 3, L"LPT") == 0) && stem[3] >= L'1' && stem[3] <= L'9';
}

// A plain file or folder name: letters, digits, space, . _ - ; no leading dot, no trailing dot/space,
// max 120, and not a reserved device name.
inline bool IsPlainMcpName(const std::wstring& name)
{
    if (name.empty() || name.size() > 120 || name.front() == L'.' || name.back() == L'.' || name.back() == L' ' || IsReservedWindowsName(name))
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
