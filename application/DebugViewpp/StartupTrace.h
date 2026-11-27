// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <windows.h>
#include <cstdio>
#include <cstdlib>

namespace fusion {
namespace debugviewpp {

// Startup timing, off unless the environment variable UDV_STARTUP_TRACE is set. Each mark appends
// "<ms since process start> <label>" to %TEMP%\udv_startup.log.
inline void StartupMark(const char* label)
{
    static const bool enabled = std::getenv("UDV_STARTUP_TRACE") != nullptr;
    if (!enabled)
        return;
    FILETIME created, exited, kernel, user, now;
    ::GetProcessTimes(::GetCurrentProcess(), &created, &exited, &kernel, &user);
    ::GetSystemTimeAsFileTime(&now);  // the build targets Windows 7: no Precise variant
    ULARGE_INTEGER c{created.dwLowDateTime, created.dwHighDateTime};
    ULARGE_INTEGER n{now.dwLowDateTime, now.dwHighDateTime};
    double ms = static_cast<double>(n.QuadPart - c.QuadPart) / 10000.0;
    wchar_t path[MAX_PATH];
    DWORD len = ::GetTempPathW(MAX_PATH, path);
    if (len == 0 || len > MAX_PATH - 20)
        return;
    wcscat_s(path, L"udv_startup.log");
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"a") == 0 && f != nullptr)
    {
        std::fprintf(f, "%9.1f %s\n", ms, label);
        std::fclose(f);
    }
}

} // namespace debugviewpp
} // namespace fusion
