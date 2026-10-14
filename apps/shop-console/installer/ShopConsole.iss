; Inno Setup script for the single-file Shop Console installer
; (ShopConsole-Setup.exe) — what gets handed to a shop on a USB stick.
; Built by .github/workflows/release.yml from the same dist\ShopConsole
; folder that goes into the self-update zip:
;
;   ISCC /DAppVersion=1.4.1 /DSourceDir=<dist\ShopConsole> /O<outdir> ShopConsole.iss
;
; Installs per-user (no admin prompt) into %LOCALAPPDATA%\Programs — the
; self-updater (scripts/update.ps1) overwrites files in place, which would
; need admin rights under Program Files.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\..\dist\ShopConsole"
#endif

[Setup]
; Never change AppId — it's how reinstalls find the existing install.
AppId={{2B241049-334F-4A3C-8F2F-E2E7385FA7BB}
AppName=Shop Console
AppVersion={#AppVersion}
AppPublisher=Shelby
DefaultDirName={autopf}\ShopConsole
DefaultGroupName=Shop Console
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputBaseFilename=ShopConsole-Setup
UninstallDisplayIcon={app}\ShopConsole.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; .env.example is for hand setup only — release builds have the connection
; baked in, and a .env a shop already has is left alone (never shipped here).
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: ".env,.env.example"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Shop Console"; Filename: "{app}\ShopConsole.exe"
Name: "{autodesktop}\Shop Console"; Filename: "{app}\ShopConsole.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\ShopConsole.exe"; Description: "{cm:LaunchProgram,Shop Console}"; Flags: nowait postinstall skipifsilent
