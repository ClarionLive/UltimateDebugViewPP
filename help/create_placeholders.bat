@echo off
setlocal enabledelayedexpansion

REM Create placeholder HTML files for help topics

set "template=topics\_template.html"
set "topics_dir=topics"

REM List of topics to create
set topics[0]=installation
set topics[1]=first-run
set topics[2]=main-window
set topics[3]=views-tabs
set topics[4]=detail-pane
set topics[5]=message-sources
set topics[6]=win32-capture
set topics[7]=file-monitoring
set topics[8]=network-sources
set topics[9]=debugview-agent
set topics[10]=filtering-basics
set topics[11]=include-exclude
set topics[12]=highlighting
set topics[13]=regex-patterns
set topics[14]=saved-filters
set topics[15]=sql-formatting
set topics[16]=sql-detection
set topics[17]=sql-options
set topics[18]=clarion-overview
set topics[19]=clarion-api
set topics[20]=clarion-messages
set topics[21]=clarion-examples
set topics[22]=syntax-highlighting
set topics[23]=bookmarks
set topics[24]=export-options
set topics[25]=performance
set topics[26]=remote-debugging
set topics[27]=command-line
set topics[28]=configuration
set topics[29]=troubleshooting
set topics[30]=faq
set topics[31]=api-methods
set topics[32]=api-properties
set topics[33]=about

REM Create each file
set i=0
:loop
if defined topics[!i!] (
    set "topic=!topics[%i%]!"
    set "filename=%topics_dir%\!topic!.html"

    REM Check if file already exists
    if not exist "!filename!" (
        echo Creating !filename!...

        REM Create title from filename (replace hyphens with spaces and capitalize)
        set "title=!topic:-= !"

        REM Copy template and replace placeholders
        powershell -Command "(Get-Content '%template%') -replace 'PAGE_TITLE', '!title!' | Set-Content '!filename!'"
    ) else (
        echo Skipping !filename! - already exists
    )

    set /a i+=1
    goto loop
)

echo.
echo Placeholder files created successfully!
echo You can now edit each file to add specific content.
pause