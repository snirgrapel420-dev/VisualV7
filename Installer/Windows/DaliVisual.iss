; =============================================================================
;  DALI VISUAL — Windows installer (Inno Setup 6)
;  Installs:
;    • VST3        →  C:\Program Files\Common Files\VST3\Dali Visual.vst3   (where every DAW looks)
;    • Standalone  →  C:\Program Files\Dali Audio\Dali Visual\Dali Visual.exe  + Start-menu / desktop shortcut
;  The user can untick either part. A normal uninstaller is registered in "Apps & features".
;
;  Built by the CI:
;    ISCC.exe /DAppVersion=7.2.0 /DBuildDir=<…\DaliVisual_artefacts\Release> /DOutDir=<out> DaliVisual.iss
; =============================================================================

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build\DaliVisual_artefacts\Release"
#endif
#ifndef OutDir
  #define OutDir "Output"
#endif

#define AppName      "Dali Visual"
#define AppPublisher "Dali Audio"
#define AppURL       "https://daliaudio.com"
#define AppExe       "Dali Visual.exe"

[Setup]
; a fixed AppId: every new version upgrades the previous installation in place
AppId={{7D3B9E52-4C1A-4F6B-9A8E-DA11A0D10F01}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName={#AppPublisher}
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExe}
OutputDir={#OutDir}
OutputBaseFilename=DaliVisual-{#AppVersion}-Windows-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; the plug-in goes to Common Files\VST3, so the installer needs administrator rights
PrivilegesRequired=admin
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
MinVersion=10.0
; a DAW or the standalone that is still open is offered to be closed before the files are replaced
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
VersionInfoProductName={#AppName}
VersionInfoDescription={#AppName} installer

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full";   Description: "Full installation (VST3 plug-in + Standalone app)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3 plug-in (for your DAW)";        Types: full custom
Name: "standalone"; Description: "Standalone application";            Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the Standalone app"; GroupDescription: "Shortcuts:"; Components: standalone

[InstallDelete]
; a clean plug-in bundle on every update (no stale files from an older version)
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3

[Files]
; VST3 bundle (a folder) — copied whole into the system VST3 folder
Source: "{#BuildDir}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3; \
    Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone application
Source: "{#BuildDir}\Standalone\{#AppExe}"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}";   Filename: "{app}\{#AppExe}"; Components: standalone
Name: "{autodesktop}\{#AppName}";    Filename: "{app}\{#AppExe}"; Components: standalone; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "Launch {#AppName} now"; Components: standalone; \
    Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"

[Messages]
FinishedLabel=Setup has installed [name] on your computer.%n%nThe VST3 plug-in is in Common Files\VST3 — open your DAW and rescan plug-ins if it does not appear straight away.
