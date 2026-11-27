// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <optional>
#include <vector>

#include "McpAgentSetup.h"

namespace fusion {
namespace debugviewpp {

namespace {

constexpr DWORD CommandTimeoutMs = 60 * 1000;
constexpr DWORD CommandNotFound = 9009;  // cmd.exe: "is not recognized as an internal or external command"

struct CommandResult
{
    DWORD exitCode = 1;
    bool timedOut = false;
};

std::wstring GetEnv(const wchar_t* name)
{
    DWORD size = ::GetEnvironmentVariableW(name, nullptr, 0);
    if (size == 0)
        return {};
    std::wstring value(size, L'\0');
    size = ::GetEnvironmentVariableW(name, value.data(), size);
    value.resize(size);
    return value;
}

bool StartsWithNoCase(const std::wstring& s, const std::wstring& prefix)
{
    return s.size() >= prefix.size() && ::_wcsnicmp(s.c_str(), prefix.c_str(), prefix.size()) == 0;
}

// This process's environment with CLAUDE_CONFIG_DIR replaced (or removed when configDir is empty).
std::vector<wchar_t> MakeEnvironment(const std::wstring& configDir)
{
    std::vector<wchar_t> block;
    if (wchar_t* env = ::GetEnvironmentStringsW())
    {
        for (const wchar_t* p = env; *p != L'\0'; p += wcslen(p) + 1)
        {
            std::wstring entry(p);
            if (StartsWithNoCase(entry, L"CLAUDE_CONFIG_DIR="))
                continue;
            block.insert(block.end(), entry.begin(), entry.end());
            block.push_back(L'\0');
        }
        ::FreeEnvironmentStringsW(env);
    }
    if (!configDir.empty())
    {
        std::wstring entry = L"CLAUDE_CONFIG_DIR=" + configDir;
        block.insert(block.end(), entry.begin(), entry.end());
        block.push_back(L'\0');
    }
    block.push_back(L'\0');
    return block;
}

// Runs "cmd.exe /d /s /c <command>" hidden (claude and codex are usually npm .cmd shims), discarding
// its output. Every argument in command is built here from fixed text, a port number and a checked token.
CommandResult RunCommand(const std::wstring& command, const std::wstring& configDir)
{
    CommandResult result;
    std::wstring comspec = GetEnv(L"ComSpec");
    if (comspec.empty())
        comspec = L"cmd.exe";
    std::wstring commandLine = L"\"" + comspec + L"\" /d /s /c \"" + command + L"\"";
    auto environment = MakeEnvironment(configDir);

    SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};
    HANDLE nul = ::CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
    if (nul == INVALID_HANDLE_VALUE)
        return result;

    // The child inherits the NUL handle and nothing else (not the server's sockets).
    SIZE_T attributeSize = 0;
    ::InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeSize);
    std::vector<BYTE> attributeBuffer(attributeSize);
    auto attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeBuffer.data());
    if (::InitializeProcThreadAttributeList(attributes, 1, 0, &attributeSize) == FALSE)
    {
        ::CloseHandle(nul);
        return result;
    }
    if (::UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &nul, sizeof(nul), nullptr, nullptr) == FALSE)
    {
        ::DeleteProcThreadAttributeList(attributes);
        ::CloseHandle(nul);
        return result;
    }

    // A job, so a timeout ends the whole tree (cmd, node, claude), not only cmd.
    HANDLE job = ::CreateJobObjectW(nullptr, nullptr);
    if (job != nullptr)
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        ::SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
    }

    STARTUPINFOEXW si = {};
    si.StartupInfo.cb = sizeof(si);
    si.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.StartupInfo.wShowWindow = SW_HIDE;
    si.StartupInfo.hStdInput = nul;
    si.StartupInfo.hStdOutput = nul;
    si.StartupInfo.hStdError = nul;
    si.lpAttributeList = attributes;

    // Start in the system folder: cmd looks for claude.cmd / codex.cmd in the current folder before PATH.
    wchar_t systemDir[MAX_PATH] = {};
    UINT systemDirLength = ::GetSystemDirectoryW(systemDir, MAX_PATH);
    PROCESS_INFORMATION pi = {};
    BOOL started = ::CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT | EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED, environment.data(),
        systemDirLength > 0 && systemDirLength < MAX_PATH ? systemDir : nullptr, &si.StartupInfo, &pi);
    ::DeleteProcThreadAttributeList(attributes);
    ::CloseHandle(nul);
    if (started == FALSE)
    {
        if (job != nullptr)
            ::CloseHandle(job);
        return result;
    }

    if (job != nullptr)
        ::AssignProcessToJobObject(job, pi.hProcess);
    ::ResumeThread(pi.hThread);
    ::CloseHandle(pi.hThread);
    if (::WaitForSingleObject(pi.hProcess, CommandTimeoutMs) == WAIT_TIMEOUT)
    {
        result.timedOut = true;
        if (job != nullptr)
            ::TerminateJobObject(job, 1);
        else
            ::TerminateProcess(pi.hProcess, 1);
    }
    else
    {
        ::GetExitCodeProcess(pi.hProcess, &result.exitCode);
    }
    ::CloseHandle(pi.hProcess);
    if (job != nullptr)
        ::CloseHandle(job);  // KILL_ON_JOB_CLOSE: also ends anything cmd left running
    return result;
}

struct ClaudeProfile
{
    std::wstring label;      // shown to the user
    std::wstring configDir;  // empty = the default profile (~/.claude.json)
};

// The default profile, every %USERPROFILE%\.claude-* folder that holds a .claude.json, and the
// viewer's own CLAUDE_CONFIG_DIR if it has one.
std::vector<ClaudeProfile> ClaudeProfiles()
{
    std::vector<ClaudeProfile> profiles{{L"default profile", L""}};
    std::wstring home = GetEnv(L"USERPROFILE");
    auto add = [&profiles](const std::wstring& dir, const std::wstring& label) {
        bool known = std::any_of(profiles.begin(), profiles.end(), [&dir](const ClaudeProfile& p) { return ::_wcsicmp(p.configDir.c_str(), dir.c_str()) == 0; });
        if (!known && ::GetFileAttributesW((dir + L"\\.claude.json").c_str()) != INVALID_FILE_ATTRIBUTES)
            profiles.push_back({label, dir});
    };
    if (!home.empty())
    {
        WIN32_FIND_DATAW data;
        HANDLE find = ::FindFirstFileW((home + L"\\.claude-*").c_str(), &data);
        if (find != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
                    add(home + L"\\" + data.cFileName, data.cFileName);
            } while (::FindNextFileW(find, &data) != FALSE);
            ::FindClose(find);
        }
    }
    std::wstring own = GetEnv(L"CLAUDE_CONFIG_DIR");
    if (!own.empty())
        add(own, own);
    return profiles;
}

std::wstring Join(const std::vector<std::wstring>& items)
{
    std::wstring text;
    for (const auto& item : items)
        text += (text.empty() ? L"" : L", ") + item;
    return text;
}

std::wstring ServerUrl(int port)
{
    return L"http://127.0.0.1:" + std::to_wstring(port) + L"/mcp";
}

// Claude Code. Removing first makes setup idempotent and picks up a changed port.
std::wstring ClaudeCode(std::optional<int> port, const std::wstring& token)
{
    std::vector<std::wstring> done;
    std::vector<std::wstring> failed;
    for (const auto& profile : ClaudeProfiles())
    {
        auto removed = RunCommand(std::wstring(L"claude mcp remove --scope user ") + McpAgentServerName, profile.configDir);
        if (removed.exitCode == CommandNotFound)
            return L"Claude Code: not installed (claude was not found).";
        if (!port)
        {
            done.push_back(profile.label);  // removing an entry that was not there is fine
            continue;
        }
        std::wstring add = std::wstring(L"claude mcp add --scope user --transport http ") + McpAgentServerName + L" " + ServerUrl(*port);
        if (!token.empty())
            add += L" --header \"Authorization: Bearer " + token + L"\"";
        auto added = RunCommand(add, profile.configDir);
        (added.exitCode == 0 && !added.timedOut ? done : failed).push_back(profile.label);
    }
    std::wstring text = port ? L"Claude Code: registered for every folder (" + Join(done) + L")." : L"Claude Code: removed (" + Join(done) + L").";
    if (done.empty())
        text = L"Claude Code: failed.";
    if (!failed.empty())
        text += L" Failed: " + Join(failed) + L".";
    return text;
}

std::wstring Codex(std::optional<int> port, bool tokenRequired)
{
    auto removed = RunCommand(std::wstring(L"codex mcp remove ") + McpAgentServerName, L"");
    if (removed.exitCode == CommandNotFound)
        return L"Codex: not installed.";
    if (!port)
        return L"Codex: removed.";
    if (tokenRequired)
        return L"Codex: skipped (it cannot send the access token; turn off \"Also require an access token\").";
    auto added = RunCommand(std::wstring(L"codex mcp add ") + McpAgentServerName + L" --url " + ServerUrl(*port), L"");
    return added.exitCode == 0 && !added.timedOut ? L"Codex: registered." : L"Codex: failed.";
}

} // namespace

bool IsCommandLineSafeToken(const std::wstring& token)
{
    return std::all_of(token.begin(), token.end(), [](wchar_t c) {
        return (c < 128 && ::iswalnum(c) != 0) || c == L'-' || c == L'_' || c == L'.' || c == L'~' || c == L'+' || c == L'/' || c == L'=';
    });
}

std::wstring SetUpAiAgents(int port, const std::wstring& token)
{
    if (!IsCommandLineSafeToken(token))
        return L"The access token has characters that cannot be passed to the agents' setup commands. Press Generate for a new one.";
    return ClaudeCode(port, token) + L" " + Codex(port, !token.empty()) + L" Agents that are already running pick it up when they next start.";
}

std::wstring RemoveAiAgents()
{
    return ClaudeCode(std::nullopt, L"") + L" " + Codex(std::nullopt, false);
}

} // namespace debugviewpp
} // namespace fusion
