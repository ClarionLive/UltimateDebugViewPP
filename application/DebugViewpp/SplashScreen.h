// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <windows.h>

namespace fusion {
namespace debugviewpp {

// "Loading" window shown while the main window is built (hidden). It runs its own thread and
// message loop, so it keeps painting while the UI thread is busy creating the views.
// All functions are safe to call from any thread and when the splash is not running.
namespace SplashScreen {

void Start(HINSTANCE instance, const std::wstring& version);
void Progress(int percent, const std::wstring& text);
void Close();

} // namespace SplashScreen

} // namespace debugviewpp
} // namespace fusion
