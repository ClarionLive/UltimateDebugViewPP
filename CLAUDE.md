# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DebugView++ is a Windows application for viewing, filtering, and analyzing debug output messages. It's an enhanced version of the classic Sysinternals DebugView tool with additional features like:
- Support for multiple log sources (Win32 OutputDebugString, files, sockets, pipes)
- Advanced filtering and highlighting
- Tabbed views and process tracking
- High-performance message handling

## Build System

The project uses CMake with Visual Studio 2022:

```bash
# Generate Visual Studio solution
cmake -DCMAKE_INSTALL_PREFIX=install -G "Visual Studio 17 2022" -B vs2022 .

# Alternative: Use the build script
build.bat

# Build with Ninja (faster)
build_with_ninja.bat
```

## Running Tests

```bash
# Run all tests via CTest
ctest --test-dir vs2022

# Run specific test executable
bin\DebugviewppTest.exe
```

## Project Architecture

### Core Components

1. **DebugViewpp** (application/DebugViewpp/): Main GUI application using WTL (Windows Template Library)
   - MainFrame.cpp: Main window and message handling
   - LogView.cpp: Log display and interaction
   - FilterDlg.cpp: Filter configuration dialogs

2. **DebugViewppLib** (application/DebugViewppLib/): Core logging library
   - LogSource.cpp: Base class for all log sources
   - DBWinReader.cpp: Reads Win32 debug messages
   - FileReader.cpp: Tails log files
   - SocketReader.cpp: TCP/UDP listeners
   - Filter.cpp: Message filtering logic
   - LineBuffer.cpp: Efficient message storage

3. **DebugViewConsole** (application/DebugViewConsole/): Command-line version for piping

4. **CobaltFusion** (application/CobaltFusion/): Reusable utilities
   - Executor.cpp: Thread pool implementation
   - CircularBuffer.cpp: Lock-free circular buffer

5. **Win32Lib** (application/Win32Lib/): Windows-specific utilities

### Key Design Patterns

- **LogSource hierarchy**: All input sources inherit from LogSource base class
- **Circular buffer**: High-performance message passing between threads
- **Filter chains**: Include/exclude/highlight filters applied in sequence
- **View separation**: Multiple independent filtered views of same log data

## Code Style

- Uses modern C++ (C++23 standard)
- Windows-specific code (Win32 API, WTL framework)
- Static runtime linking (/MT) to avoid dependencies
- Unicode builds only
- Follows existing code patterns when modifying files