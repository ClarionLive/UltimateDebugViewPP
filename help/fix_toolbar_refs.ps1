# PowerShell script to fix all Ribbon references and update with correct toolbar documentation

$topicsPath = "C:\Dev\UltimateDebugViewPP\bin\Debug\help\topics"

# First, let's rename the ribbon-interface.html file to toolbar.html
if (Test-Path "$topicsPath\ribbon-interface.html") {
    Remove-Item "$topicsPath\ribbon-interface.html" -Force
}

# Create new toolbar.html with correct content
@'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Toolbar and Menus - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Toolbar and Menus</h1>
    <p class="lead">Navigate UltimateDebugView++ using the toolbar and menu system.</p>

    <h2>Toolbar Overview</h2>
    <p>The toolbar provides quick access to frequently used functions. Buttons include:</p>
    <ul>
        <li><strong>Clear:</strong> Clear all messages from the current view</li>
        <li><strong>Save:</strong> Save captured messages to file</li>
        <li><strong>View Log:</strong> Open saved log files</li>
        <li><strong>Global Win32:</strong> Toggle global Win32 OutputDebugString capture</li>
        <li><strong>Capture:</strong> Start/stop message capture</li>
        <li><strong>Auto Scroll:</strong> Toggle automatic scrolling to new messages</li>
        <li><strong>Time:</strong> Show/hide timestamp column</li>
        <li><strong>Font:</strong> Change display font</li>
        <li><strong>Filter:</strong> Manage message filters</li>
        <li><strong>Link Views:</strong> Synchronize multiple view tabs</li>
        <li><strong>Bookmark:</strong> Set/navigate bookmarks</li>
        <li><strong>Process Colors:</strong> Color-code messages by process</li>
    </ul>

    <h2>Expanded Toolbar Feature</h2>
    <p>For improved readability, enable the Expanded Toolbar option:</p>
    <ol>
        <li>Go to <strong>Options &rarr; Expanded Toolbar</strong></li>
        <li>The toolbar switches to a more readable format with text labels</li>
        <li>Each button displays both icon and descriptive text</li>
        <li>Ideal for high-resolution displays or when learning the interface</li>
    </ol>

    <h3>Benefits of Expanded Toolbar</h3>
    <ul>
        <li>Clearer button identification</li>
        <li>Reduced learning curve for new users</li>
        <li>Better accessibility</li>
        <li>No need to memorize icon meanings</li>
    </ul>

    <h2>Menu Bar</h2>
    <p>The menu bar organizes all features into logical categories:</p>

    <h3>File Menu</h3>
    <ul>
        <li><strong>New Tab:</strong> Create a new message view</li>
        <li><strong>Open:</strong> Load saved log files</li>
        <li><strong>Save Log:</strong> Save current messages</li>
        <li><strong>Save View:</strong> Save view configuration</li>
        <li><strong>Load Configuration:</strong> Load saved settings</li>
        <li><strong>Run:</strong> Execute external programs</li>
        <li><strong>Exit:</strong> Close application</li>
    </ul>

    <h3>Edit Menu</h3>
    <ul>
        <li><strong>Copy:</strong> Copy selected messages</li>
        <li><strong>Select All:</strong> Select all messages</li>
        <li><strong>Find:</strong> Search within messages</li>
        <li><strong>Filter:</strong> Quick filter options</li>
    </ul>

    <h3>Log Menu</h3>
    <ul>
        <li><strong>Sources:</strong> Manage message sources</li>
        <li><strong>History:</strong> View message history</li>
        <li><strong>Highlight Filters:</strong> Configure highlighting</li>
        <li><strong>Process Include/Exclude:</strong> Filter by process</li>
        <li><strong>Clear View:</strong> Clear current view</li>
    </ul>

    <h3>View Menu</h3>
    <ul>
        <li><strong>Process Colors:</strong> Enable process coloring</li>
        <li><strong>Auto Scroll:</strong> Toggle auto-scroll</li>
        <li><strong>Show Timestamps:</strong> Toggle time column</li>
        <li><strong>Filter:</strong> Show filter dialog</li>
        <li><strong>Find:</strong> Search messages</li>
        <li><strong>Bookmarks:</strong> Manage bookmarks</li>
    </ul>

    <h3>Options Menu</h3>
    <ul>
        <li><strong>Link Views:</strong> Synchronize view navigation</li>
        <li><strong>Auto Newline:</strong> Append newlines to messages</li>
        <li><strong>Prefix PID:</strong> Add process ID prefix</li>
        <li><strong>Font:</strong> Configure display font</li>
        <li><strong>Always On Top:</strong> Keep window on top</li>
        <li><strong>Hide When Minimized:</strong> Hide in system tray</li>
        <li><strong>Expanded Toolbar:</strong> Toggle expanded toolbar with text labels</li>
        <li><strong>Import DebugView++ Settings:</strong> Import settings from DebugView++</li>
    </ul>

    <h3>Help Menu</h3>
    <ul>
        <li><strong>Help Topics:</strong> Open this help system</li>
        <li><strong>About:</strong> Version and credits</li>
    </ul>

    <h2>Toolbar Customization</h2>
    <p>The toolbar automatically adjusts to window size:</p>
    <ul>
        <li>Icons only in compact mode (default)</li>
        <li>Icons with text labels in expanded mode</li>
        <li>Automatic wrapping on narrow windows</li>
        <li>Settings persist between sessions</li>
    </ul>

    <h2>Keyboard Access</h2>
    <p>Access toolbar functions via keyboard:</p>
    <ul>
        <li><strong>Alt key:</strong> Activate menu bar</li>
        <li><strong>Accelerators:</strong> Use underlined letters in menus</li>
        <li><strong>Shortcuts:</strong> Direct keyboard shortcuts for common functions</li>
    </ul>

    <div class="note">
        <strong>Tip:</strong> Enable Expanded Toolbar through Options menu for better readability, especially on high-DPI displays.
    </div>

    <div class="topic-nav">
        <a href="quick-start.html" class="nav-prev">Previous: Quick Start</a>
        <a href="main-window.html" class="nav-next">Next: Main Window</a>
    </div>
</body>
</html>
'@ | Set-Content "$topicsPath\toolbar.html" -Encoding UTF8

Write-Host "Created toolbar.html"

# Update index.html to replace Ribbon Interface with Toolbar
$indexContent = Get-Content "C:\Dev\UltimateDebugViewPP\bin\Debug\help\index.html" -Raw
$indexContent = $indexContent -replace 'href="topics/ribbon-interface.html"[^>]*>Ribbon Interface', 'href="topics/toolbar.html" target="contentFrame" class="nav-link">Toolbar and Menus'
$indexContent | Set-Content "C:\Dev\UltimateDebugViewPP\bin\Debug\help\index.html" -Encoding UTF8
Write-Host "Updated index.html"

# Update all files with ribbon references
$files = @(
    "first-run.html",
    "main-window.html",
    "welcome.html",
    "quick-start.html",
    "filtering-basics.html",
    "message-sources.html",
    "win32-capture.html",
    "file-monitoring.html",
    "network-sources.html",
    "debugview-agent.html",
    "about.html",
    "faq.html",
    "keyboard-shortcuts.html",
    "saved-filters.html",
    "include-exclude.html",
    "sql-formatting.html",
    "clarion-overview.html"
)

foreach ($file in $files) {
    $filePath = "$topicsPath\$file"
    if (Test-Path $filePath) {
        $content = Get-Content $filePath -Raw

        # Common replacements
        $content = $content -replace 'Modern Ribbon UI', 'Classic toolbar with Expanded mode option'
        $content = $content -replace 'Modern Ribbon interface', 'Intuitive toolbar with optional expanded mode'
        $content = $content -replace 'modern ribbon UI', 'intuitive toolbar interface'
        $content = $content -replace 'ribbon interface', 'toolbar and menu system'
        $content = $content -replace 'Ribbon Interface', 'Toolbar and Menus'
        $content = $content -replace 'Ribbon interface', 'toolbar'
        $content = $content -replace 'the ribbon', 'the toolbar'
        $content = $content -replace 'on the ribbon', 'on the toolbar or in the menu'

        # File-specific replacements
        switch ($file) {
            "first-run.html" {
                $content = $content -replace '<strong>Home</strong> tab on the ribbon', 'toolbar buttons or <strong>Log</strong> menu'
                $content = $content -replace '<strong>Filter</strong> tab', '<strong>Filter</strong> button on the toolbar'
                $content = $content -replace 'ribbon-interface\.html', 'toolbar.html'
            }
            "main-window.html" {
                $content = $content -replace 'modern ribbon interface at the top[^.]*\.', 'toolbar provides quick access to frequently used functions.'
                $content = $content -replace 'Below the ribbon', 'Below the toolbar'
            }
            "filtering-basics.html" {
                $content = $content -replace '<strong>Filter</strong> tab on ribbon', '<strong>Filter</strong> button on the toolbar or Edit menu'
                $content = $content -replace 'Navigate to <strong>Filter</strong> tab', 'Click the <strong>Filter</strong> button'
            }
            "message-sources.html" {
                $content = $content -replace '<strong>Home</strong> tab on the ribbon', '<strong>Log</strong> menu'
                $content = $content -replace 'appropriate source button[^)]*\)', 'appropriate option from the Log menu'
            }
            "saved-filters.html" {
                $content = $content -replace 'Filter .{1,3} Save Filter Set', 'Edit menu - Save Filter Set'
                $content = $content -replace 'Filter .{1,3} Load Filter Set', 'Edit menu - Load Filter Set'
            }
        }

        $content | Set-Content $filePath -Encoding UTF8
        Write-Host "Updated $file"
    }
}

Write-Host "`nAll Ribbon references have been removed!"
Write-Host "Toolbar and Expanded Toolbar feature are now properly documented."