@echo off
echo Creating all missing help topic files...

REM Capturing Messages section
echo Creating message-sources.html...
call :create_file "message-sources.html" "Message Sources" "Overview of all available message sources in UltimateDebugView++."

echo Creating win32-capture.html...
call :create_file "win32-capture.html" "Win32 OutputDebugString Capture" "Capturing debug output from Windows applications using OutputDebugString API."

echo Creating file-monitoring.html...
call :create_file "file-monitoring.html" "File Monitoring" "Monitor and tail log files in real-time."

echo Creating network-sources.html...
call :create_file "network-sources.html" "Network Sources" "Receive debug messages over TCP/UDP network connections."

echo Creating debugview-agent.html...
call :create_file "debugview-agent.html" "DebugView Agent" "Remote debugging with the DebugView Agent."

REM Filtering section
echo Creating filtering-basics.html...
call :create_file "filtering-basics.html" "Filtering Basics" "Learn how to filter debug messages effectively."

echo Creating include-exclude.html...
call :create_file "include-exclude.html" "Include/Exclude Filters" "Control which messages are displayed using include and exclude filters."

echo Creating highlighting.html...
call :create_file "highlighting.html" "Message Highlighting" "Highlight important messages with colors and styles."

echo Creating regex-patterns.html...
call :create_file "regex-patterns.html" "Regular Expression Patterns" "Use powerful regex patterns for advanced filtering."

echo Creating saved-filters.html...
call :create_file "saved-filters.html" "Saved Filters" "Save and manage filter sets for different debugging scenarios."

REM SQL Features
echo Creating sql-formatting.html...
call :create_file "sql-formatting.html" "SQL Formatting" "Automatic SQL query beautification and formatting."

echo Creating sql-detection.html...
call :create_file "sql-detection.html" "Automatic SQL Detection" "How UltimateDebugView++ detects and processes SQL statements."

echo Creating sql-options.html...
call :create_file "sql-options.html" "SQL Formatting Options" "Configure SQL formatting preferences and styles."

REM Clarion Integration
echo Creating clarion-overview.html...
call :create_file "clarion-overview.html" "Clarion Integration Overview" "Using UltimateDebugView++ with Clarion applications."

echo Creating clarion-api.html...
call :create_file "clarion-api.html" "UltimateDebug Class API" "Complete API reference for the UltimateDebug Clarion class."

echo Creating clarion-messages.html...
call :create_file "clarion-messages.html" "Clarion Message Format" "Understanding the Clarion debug message format and structure."

echo Creating clarion-examples.html...
call :create_file "clarion-examples.html" "Clarion Code Examples" "Example code for integrating Clarion applications with UltimateDebugView++."

REM Advanced Features
echo Creating syntax-highlighting.html...
call :create_file "syntax-highlighting.html" "Syntax Highlighting" "Automatic syntax highlighting for various programming languages."

echo Creating bookmarks.html...
call :create_file "bookmarks.html" "Bookmarks" "Mark and navigate to important messages using bookmarks."

echo Creating export-options.html...
call :create_file "export-options.html" "Export Options" "Export debug messages to various formats."

echo Creating performance.html...
call :create_file "performance.html" "Performance Tips" "Optimize UltimateDebugView++ for high-volume debugging."

echo Creating remote-debugging.html...
call :create_file "remote-debugging.html" "Remote Debugging" "Debug applications on remote machines."

REM Reference section
echo Creating command-line.html...
call :create_file "command-line.html" "Command Line Options" "Command line parameters and automation options."

echo Creating configuration.html...
call :create_file "configuration.html" "Configuration Settings" "Configure UltimateDebugView++ preferences and options."

echo Creating troubleshooting.html...
call :create_file "troubleshooting.html" "Troubleshooting" "Common issues and solutions."

echo Creating faq.html...
call :create_file "faq.html" "Frequently Asked Questions" "Answers to common questions about UltimateDebugView++."

REM API Reference
echo Creating api-methods.html...
call :create_file "api-methods.html" "API Method Reference" "Complete reference of all API methods."

echo Creating api-properties.html...
call :create_file "api-properties.html" "API Properties Reference" "Complete reference of all API properties."

REM About page
echo Creating about.html...
call :create_file "about.html" "About UltimateDebugView++" "Version information and credits."

echo.
echo All files created successfully!
goto :eof

:create_file
set filename=%~1
set title=%~2
set description=%~3

(
echo ^<!DOCTYPE html^>
echo ^<html lang="en"^>
echo ^<head^>
echo     ^<meta charset="UTF-8"^>
echo     ^<meta name="viewport" content="width=device-width, initial-scale=1.0"^>
echo     ^<title^>%title% - UltimateDebugView++^</title^>
echo     ^<link rel="stylesheet" href="../css/topic-styles.css"^>
echo ^</head^>
echo ^<body^>
echo     ^<h1^>%title%^</h1^>
echo     ^<p class="lead"^>%description%^</p^>
echo.
echo     ^<div class="note"^>
echo         This documentation is being developed. In the meantime:
echo         ^<ul^>
echo             ^<li^>Explore other help topics using the navigation^</li^>
echo             ^<li^>Check the ^<a href="quick-start.html"^>Quick Start Guide^</a^>^</li^>
echo             ^<li^>Review ^<a href="keyboard-shortcuts.html"^>Keyboard Shortcuts^</a^>^</li^>
echo         ^</ul^>
echo     ^</div^>
echo.
echo     ^<h2^>Overview^</h2^>
echo     ^<p^>This section will provide comprehensive information about %title%.^</p^>
echo.
echo     ^<h2^>Key Features^</h2^>
echo     ^<ul^>
echo         ^<li^>Feature details will be added here^</li^>
echo         ^<li^>Step-by-step instructions^</li^>
echo         ^<li^>Best practices and tips^</li^>
echo     ^</ul^>
echo.
echo     ^<div class="topic-nav"^>
echo         ^<a href="#" class="nav-prev"^>Previous Topic^</a^>
echo         ^<a href="#" class="nav-next"^>Next Topic^</a^>
echo     ^</div^>
echo ^</body^>
echo ^</html^>
) > "topics\%filename%"
goto :eof