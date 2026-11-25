// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>

#include <windows.h>
#include <bcrypt.h>

namespace fusion {
namespace debugviewpp {

struct McpSettings
{
    bool enabled = false;
    int port = 7777;
    std::wstring token;  // required while enabled; generated automatically when missing
    bool maskSecrets = true;

    bool operator==(const McpSettings& other) const
    {
        return enabled == other.enabled && port == other.port && token == other.token && maskSecrets == other.maskSecrets;
    }
    bool operator!=(const McpSettings& other) const { return !(*this == other); }
};

constexpr size_t McpMinTokenLength = 16;

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
