; Inno Setup script — собирает один Setup.exe из портативной папки dist/.
; Сборка: "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\xipher.iss
#define MyApp "Xipher"
#define MyVer "0.1.0"
#define MyExe "Xipher.exe"

[Setup]
AppName={#MyApp}
AppVersion={#MyVer}
AppPublisher=Xipher
DefaultDirName={autopf}\{#MyApp}
DefaultGroupName={#MyApp}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#MyExe}
SetupIconFile=..\assets\xipher.ico
OutputDir=output
OutputBaseFilename=Xipher-Setup-{#MyVer}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\dist\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyApp}"; Filename: "{app}\{#MyExe}"
Name: "{autodesktop}\{#MyApp}"; Filename: "{app}\{#MyExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyExe}"; Description: "{cm:LaunchProgram,{#MyApp}}"; Flags: nowait postinstall skipifsilent
