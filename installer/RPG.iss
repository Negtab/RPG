; Установщик игры RPG (Inno Setup 6).
; Собирать через build_installer.ps1 — он соберёт Release и передаст пути сюда.
; Вручную: ISCC.exe /DBuildDir=..\cmake-build-release RPG.iss

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\cmake-build-release"
#endif

#define AppName    "RPG"
#define AppExe     "RPG.exe"
#define SourceRoot ".."

[Setup]
AppId={{6F3C1A52-8E4B-4D7A-9C21-5B0E7D3F9A14}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Negtab
VersionInfoVersion={#AppVersion}
; Игра пишет saves\ и settings.ini рядом с собой, поэтому ставим в профиль пользователя — без UAC
PrivilegesRequired=lowest
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\bin\{#AppExe}
OutputDir=Output
OutputBaseFilename={#AppName}-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Игра ищет resources\ в папке НАД рабочей (current_path().remove_filename()),
; поэтому exe лежит в {app}\bin, а ресурсы — в {app}\resources
Source: "{#BuildDir}\PROJECT_NAME.exe"; DestDir: "{app}\bin"; DestName: "{#AppExe}"; Flags: ignoreversion
Source: "{#BuildDir}\*.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#SourceRoot}\resources\*"; DestDir: "{app}\resources"; Excludes: "Example*.png"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\bin\{#AppExe}"; WorkingDir: "{app}\bin"
Name: "{autoprograms}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\bin\{#AppExe}"; WorkingDir: "{app}\bin"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\{#AppExe}"; WorkingDir: "{app}\bin"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent
