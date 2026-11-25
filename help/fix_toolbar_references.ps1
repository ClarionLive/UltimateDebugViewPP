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
        <li>Go to <strong>Options → Expanded Toolbar</strong></li>
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

# Update first-run.html
$content = Get-Content "$topicsPath\first-run.html" -Raw
$content = $content -replace 'Navigate to the <strong>Home</strong> tab on the ribbon', 'Use the toolbar buttons or <strong>Log</strong> menu'
$content = $content -replace 'Use the <strong>Filter</strong> tab', 'Use the <strong>Filter</strong> button on the toolbar or <strong>Edit → Filter</strong> menu'
$content = $content -replace 'Explore the <a href="ribbon-interface.html">Ribbon Interface</a>', 'Learn about the <a href="toolbar.html">Toolbar and Menus</a>'
$content | Set-Content "$topicsPath\first-run.html" -Encoding UTF8
Write-Host "Updated first-run.html"

# Update main-window.html
$content = Get-Content "$topicsPath\main-window.html" -Raw
$content = $content -replace '<h3>1\. Ribbon Interface</h3>[\s\S]*?</ul>', @'
<h3>1. Toolbar and Menu Bar</h3>
    <p>The toolbar at the top provides quick access to common functions:</p>
    <ul>
        <li><strong>Standard mode:</strong> Compact icons for space efficiency</li>
        <li><strong>Expanded mode:</strong> Icons with text labels (Options → Expanded Toolbar)</li>
        <li><strong>Menu bar:</strong> Full access to all features organized by category</li>
        <li><strong>Customizable:</strong> Settings persist between sessions</li>
    </ul>
'@
$content = $content -replace '<h3>3\. Tab Bar</h3>\s*<p>Below the ribbon', '<h3>3. Tab Bar</h3>
    <p>Below the toolbar'
$content = $content -replace 'The modern ribbon interface at the top provides access to all features organized into logical tabs:[\s\S]*?<li><strong>Tools:</strong>[^<]*</li>\s*</ul>', 'The toolbar provides quick access to frequently used functions. Use the menu bar for complete access to all features.'
$content | Set-Content "$topicsPath\main-window.html" -Encoding UTF8
Write-Host "Updated main-window.html"

# Update welcome.html
$content = Get-Content "$topicsPath\welcome.html" -Raw
$content = $content -replace 'Modern Ribbon interface for easy access to features', 'Intuitive toolbar with optional expanded mode for better readability'
$content | Set-Content "$topicsPath\welcome.html" -Encoding UTF8
Write-Host "Updated welcome.html"

# Update quick-start.html
$content = Get-Content "$topicsPath\quick-start.html" -Raw
$content = $content -replace 'Click the appropriate button in the Home ribbon tab', 'Click the appropriate toolbar button or use the Log menu'
$content = $content -replace 'Use the Filter ribbon tab', 'Click the Filter button on the toolbar'
$content = $content -replace 'ribbon', 'toolbar'
$content = $content -replace 'Ribbon', 'Toolbar'
$content | Set-Content "$topicsPath\quick-start.html" -Encoding UTF8
Write-Host "Updated quick-start.html"

# Update filtering-basics.html
$content = Get-Content "$topicsPath\filtering-basics.html" -Raw
$content = $content -replace 'Navigate to <strong>Filter</strong> tab on ribbon', 'Click the <strong>Filter</strong> button on the toolbar'
$content | Set-Content "$topicsPath\filtering-basics.html" -Encoding UTF8
Write-Host "Updated filtering-basics.html"

# Update message-sources.html
$content = Get-Content "$topicsPath\message-sources.html" -Raw
$content = $content -replace 'Navigate to the <strong>Home</strong> tab on the ribbon', 'Use the <strong>Log</strong> menu'
$content = $content -replace 'Click the appropriate source button \(File, Network, etc\.\)', 'Select the appropriate option from Log → Sources'
$content | Set-Content "$topicsPath\message-sources.html" -Encoding UTF8
Write-Host "Updated message-sources.html"

# Update win32-capture.html
$content = Get-Content "$topicsPath\win32-capture.html" -Raw
$content = $content -replace 'Or click <strong>Home → Win32 Capture</strong>', 'Or click the <strong>Global Win32</strong> button on the toolbar'
$content | Set-Content "$topicsPath\win32-capture.html" -Encoding UTF8
Write-Host "Updated win32-capture.html"

# Update file-monitoring.html
$content = Get-Content "$topicsPath\file-monitoring.html" -Raw
$content = $content -replace 'Click <strong>Home → Add File</strong> on the ribbon', 'Select <strong>Log → Sources → Add File</strong> from the menu'
$content | Set-Content "$topicsPath\file-monitoring.html" -Encoding UTF8
Write-Host "Updated file-monitoring.html"

# Update network-sources.html
$content = Get-Content "$topicsPath\network-sources.html" -Raw
$content = $content -replace 'Click <strong>Home → Add Network → UDP</strong>', 'Select <strong>Log → Sources → Add Network → UDP</strong>'
$content = $content -replace 'Click <strong>Home → Add Network → TCP Agent</strong>', 'Select <strong>Log → Sources → Add Network → TCP Agent</strong>'
$content | Set-Content "$topicsPath\network-sources.html" -Encoding UTF8
Write-Host "Updated network-sources.html"

# Update debugview-agent.html
$content = Get-Content "$topicsPath\debugview-agent.html" -Raw
$content = $content -replace 'Click <strong>Home → Remote Agent</strong>', 'Select <strong>Log → Sources → Remote Agent</strong>'
$content | Set-Content "$topicsPath\debugview-agent.html" -Encoding UTF8
Write-Host "Updated debugview-agent.html"

# Update about.html
$content = Get-Content "$topicsPath\about.html" -Raw
$content = $content -replace 'Modern Ribbon UI', 'Classic toolbar with Expanded mode option'
$content | Set-Content "$topicsPath\about.html" -Encoding UTF8
Write-Host "Updated about.html"

# Update FAQ
$content = Get-Content "$topicsPath\faq.html" -Raw
$content = $content -replace 'modern ribbon UI', 'intuitive toolbar interface'
$content | Set-Content "$topicsPath\faq.html" -Encoding UTF8
Write-Host "Updated faq.html"

# Update keyboard-shortcuts.html if it has ribbon references
if (Test-Path "$topicsPath\keyboard-shortcuts.html") {
    $content = Get-Content "$topicsPath\keyboard-shortcuts.html" -Raw
    $content = $content -replace 'ribbon', 'toolbar'
    $content = $content -replace 'Ribbon', 'Toolbar'
    $content | Set-Content "$topicsPath\keyboard-shortcuts.html" -Encoding UTF8
    Write-Host "Updated keyboard-shortcuts.html"
}

Write-Host "`nAll Ribbon references have been removed and replaced with correct toolbar documentation!"
Write-Host "The Expanded Toolbar feature is now properly documented."