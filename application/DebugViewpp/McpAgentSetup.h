// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>

namespace fusion {
namespace debugviewpp {

// Registers (or removes) the viewer's MCP server with the AI agents installed for this Windows user,
// through their own command-line tools, so no configuration file is edited by hand:
// - Claude Code at user scope (every folder and session), in the default profile and in every
//   %USERPROFILE%\.claude-* profile folder (Claude profiles that MultiTerminal launches with CLAUDE_CONFIG_DIR)
// - Codex CLI, if installed
// Both take several seconds: call them from a worker thread. They return a short report for the user.
std::wstring SetUpAiAgents(int port, const std::wstring& token);  // token: empty unless one is required
std::wstring RemoveAiAgents();

// A token can be passed on a command line only if it has no characters cmd.exe treats specially.
bool IsCommandLineSafeToken(const std::wstring& token);

constexpr const wchar_t* McpAgentServerName = L"ultimate-debugview";

} // namespace debugviewpp
} // namespace fusion
