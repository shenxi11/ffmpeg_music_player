#define MyAppName "YunMusic"
#define MyAppPublisher "CloudMusic Project"
#define MyAppExeName "ffmpeg_music_player.exe"

#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif

#ifndef SourceDir
  #error SourceDir is not defined. Use ISCC /DSourceDir=...
#endif

#ifndef OutputDir
  #define OutputDir "."
#endif

[Setup]
AppId={{9A5C0D91-9C2E-4D6A-B2A7-9C26B66DCC2A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppVerName={#MyAppName} {#MyAppVersion}
DefaultDirName={autopf}\YunMusic
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=no
UsePreviousAppDir=yes
OutputDir={#OutputDir}
OutputBaseFilename=YunMusic_Setup_{#MyAppVersion}
SetupIconFile=..\..\icon\netease.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=no
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation (recommended)"
Name: "minimal"; Description: "Minimal installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "core"; Description: "Main application"; Types: full minimal custom; Flags: fixed
Name: "plugins"; Description: "Plugin components"; Types: full custom
Name: "plugins\audio_converter"; Description: "Audio Converter plugin"; Types: full custom
Name: "plugins\whisper_translate"; Description: "Whisper Translate plugin"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Additional tasks:"
Name: "startmenuicon"; Description: "Create start menu shortcut"; GroupDescription: "Additional tasks:"; Flags: checkedonce

[InstallDelete]
; Ensure optional plugin selection is deterministic across upgrades.
Type: filesandordirs; Name: "{app}\plugin"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: core; \
    Excludes: "*.pdb,*.ilk,*.exp,*.lib,*.ipdb,*.iobj,debug.log,plugin\*"

Source: "{#SourceDir}\plugin\audio_converter_plugin.dll"; DestDir: "{app}\plugin"; Flags: ignoreversion; Components: plugins\audio_converter
Source: "{#SourceDir}\plugin\audio_converter_plugin.json"; DestDir: "{app}\plugin"; Flags: ignoreversion; Components: plugins\audio_converter

Source: "{#SourceDir}\plugin\whisper_translate_plugin.dll"; DestDir: "{app}\plugin"; Flags: ignoreversion; Components: plugins\whisper_translate
Source: "{#SourceDir}\plugin\whisper_translate_plugin.json"; DestDir: "{app}\plugin"; Flags: ignoreversion; Components: plugins\whisper_translate
Source: "{#SourceDir}\resource\*"; DestDir: "{app}\resource"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: plugins\whisper_translate

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startmenuicon
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
