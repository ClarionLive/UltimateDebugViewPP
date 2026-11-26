# Licence and third-party notices

Ultimate DebugView++ is distributed under the **Boost Software License 1.0** (see [`LICENSE`](LICENSE)).

It is a derivative work of **DebugView++**, Copyright (C) Gert-Jan de Vos and Jan Wilmans 2013 and later, https://github.com/CobaltFusion/DebugViewPP, also under the Boost Software License 1.0. The original copyright notices are kept in the source files. Additions by ClarionLive are Copyright (C) ClarionLive and released under the same licence.

The program includes or uses the components below. Their licence texts are in [`help/licenses/`](help/licenses/), which is copied next to `UltimateDebugviewpp.exe` on every build, so every copy of the program carries them.

| Component | Used for | Licence | Licence text |
|---|---|---|---|
| DebugView++ (Gert-Jan de Vos, Jan Wilmans) | the base application | Boost Software License 1.0 | `help/licenses/DebugViewPP-BSL-1.0.txt` |
| Boost 1.80 (Asio, Beast, JSON and others; header-only) | networking, JSON, utilities | Boost Software License 1.0 | `help/licenses/DebugViewPP-BSL-1.0.txt` (same text) |
| Windows Template Library (WTL) 10 | user interface | Microsoft Public License (MS-PL) | `help/licenses/WTL-MS-PL.txt` |
| Scintilla and Lexilla (Neil Hodgson) | detail pane editor and syntax highlighting | Historical Permission Notice and Disclaimer | `help/licenses/Scintilla-Lexilla-License.txt` |
| Snappy 1.1 (Google Inc.) | log storage compression | BSD 3-Clause | `help/licenses/Snappy-BSD-3-Clause.txt` |
| fsqlf, Free SQL Formatter (dnsmkl) | SQL formatting, as the separate `fsqlf.dll` | GNU LGPL v3 (supplementing the GNU GPL v3) | `help/licenses/fsqlf-LGPL-3.0.txt`, `help/licenses/GPL-3.0.txt` |
| Microsoft Edge WebView2 SDK (loader) | in-app help viewer | Microsoft WebView2 SDK licence | `help/licenses/WebView2-SDK-License.txt` |
| Tabbing Framework (Daniel Bowen) | tabbed views | Free use in compiled form (see the file header) | `application/Libraries/TabbingFramework/license.txt` |

## fsqlf (LGPL v3)

`fsqlf.dll` is a separate, dynamically loaded library. You may replace it with your own build of fsqlf.
- Its source code is in this repository under [`fsqlf_temp/`](fsqlf_temp/) (with [`fsqlf_dll/`](fsqlf_dll/) for the DLL build), and upstream at https://github.com/dnsmkl/fsqlf.
- Changes made for this project are in that folder's history.

The LGPL v3 and the GPL v3 it builds on are both included.
