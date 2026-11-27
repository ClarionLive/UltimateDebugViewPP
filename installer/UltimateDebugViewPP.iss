; Ultimate DebugView++ installer (Inno Setup 6).
; Build with installer\build-installer.cmd, which builds the program, signs it and compiles this script.
; By hand (unsigned):  ISCC.exe installer\UltimateDebugViewPP.iss
; Signed:              ISCC.exe /DSign "/Ssigntool=<signtool command> $f" installer\UltimateDebugViewPP.iss

#define AppName      "Ultimate DebugView++"
#define AppExe       "UltimateDebugviewpp.exe"
#define AppPublisher "ClarionLive"
#define AppUrl       "https://github.com/ClarionLive/UltimateDebugViewPP"
#define Root         AddBackslash(SourcePath) + ".."
#ifndef BinDir
  #define BinDir     Root + "\bin\Release"
#endif
; The version comes from the built exe (application\DebugViewpp\version.h).
#define AppVersion   GetVersionNumbersString(BinDir + "\" + AppExe)
#if AppVersion == ""
  #error Build the Release exe first (bin\Release\UltimateDebugviewpp.exe not found)
#endif

[Setup]
AppId={{72D1DB66-DC57-44ED-856F-FC06D05A5818}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppUrl}
AppSupportURL={#AppUrl}/issues
AppUpdatesURL={#AppUrl}/releases
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} Setup
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
; Program Files for everyone (admin); the wizard also offers "install for me only" without admin.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=6.1sp1
LicenseFile={#Root}\LICENSE
SetupIconFile={#Root}\application\DebugViewpp\res\DebugView++.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
OutputDir={#Root}\installer\Output
OutputBaseFilename=UltimateDebugViewPP-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; Close a running viewer (Restart Manager) instead of failing on a locked exe.
CloseApplications=yes
RestartApplications=no
#ifdef Sign
SignTool=signtool
SignedUninstaller=yes
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "webview2"; Description: "Download and install the Microsoft Edge WebView2 Runtime (needed for the in-app help)"; GroupDescription: "Requirements:"; Check: not IsWebView2Installed

[Files]
Source: "{#BinDir}\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BinDir}\fsqlf.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BinDir}\help\*"; DestDir: "{app}\help"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Root}\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "{#Root}\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion

[InstallDelete]
; Files from older builds that are no longer shipped.
Type: filesandordirs; Name: "{app}\help"

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; WebView2 data of the in-app help, kept per user under Local AppData.
Type: filesandordirs; Name: "{localappdata}\UltimateDebugView++\WebView2"
Type: dirifempty; Name: "{localappdata}\UltimateDebugView++"

[Code]
const
  WebView2ClientKey = 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}';
  WebView2BootstrapperUrl = 'https://go.microsoft.com/fwlink/p/?LinkId=2124703';
  SettingsKey = 'Software\ClarionLive\UltimateDebugView++';

function WebView2VersionIn(RootKey: Integer; SubKey: String): Boolean;
var
  Version: String;
begin
  Result := RegQueryStringValue(RootKey, SubKey, 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0');
end;

// Microsoft's documented check: the runtime's "pv" value, per machine (32-bit view) or per user.
function IsWebView2Installed: Boolean;
begin
  Result := WebView2VersionIn(HKLM32, WebView2ClientKey) or
            WebView2VersionIn(HKLM64, WebView2ClientKey) or
            WebView2VersionIn(HKCU, WebView2ClientKey);
end;

procedure InstallWebView2;
var
  ResultCode: Integer;
  Bootstrapper: String;
begin
  WizardForm.StatusLabel.Caption := 'Downloading the Microsoft Edge WebView2 Runtime...';
  try
    DownloadTemporaryFile(WebView2BootstrapperUrl, 'MicrosoftEdgeWebview2Setup.exe', '', nil);
    Bootstrapper := ExpandConstant('{tmp}\MicrosoftEdgeWebview2Setup.exe');
    WizardForm.StatusLabel.Caption := 'Installing the Microsoft Edge WebView2 Runtime...';
    if not Exec(Bootstrapper, '/silent /install', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
      MsgBox('The WebView2 Runtime could not be installed (code ' + IntToStr(ResultCode) + '). ' +
             'Ultimate DebugView++ works without it, but the in-app help needs it. ' +
             'You can install it later from https://developer.microsoft.com/microsoft-edge/webview2/', mbInformation, MB_OK);
  except
    MsgBox('The WebView2 Runtime could not be downloaded: ' + GetExceptionMessage + #13#10 +
           'Ultimate DebugView++ works without it, but the in-app help needs it.', mbInformation, MB_OK);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('webview2') then
    InstallWebView2;
end;

// Settings (filters, views, AI / MCP options) live in the registry of each user. Keep them unless the
// user says otherwise; a silent uninstall always keeps them.
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if (CurUninstallStep = usPostUninstall) and not UninstallSilent and RegKeyExists(HKCU, SettingsKey) then
  begin
    if MsgBox('Also remove your Ultimate DebugView++ settings (filters, views, AI / MCP options)?' + #13#10 + #13#10 +
              'Choose No to keep them for a later install.', mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
      RegDeleteKeyIncludingSubkeys(HKCU, SettingsKey);
  end;
end;
