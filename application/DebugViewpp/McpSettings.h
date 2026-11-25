// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>

namespace fusion {
namespace debugviewpp {

struct McpSettings
{
    bool enabled = false;
    int port = 7777;
    std::wstring token;
    bool maskSecrets = true;

    bool operator==(const McpSettings& other) const
    {
        return enabled == other.enabled && port == other.port && token == other.token && maskSecrets == other.maskSecrets;
    }
    bool operator!=(const McpSettings& other) const { return !(*this == other); }
};

} // namespace debugviewpp
} // namespace fusion
