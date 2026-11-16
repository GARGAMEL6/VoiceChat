; Скрипт установщика для VoiceClient
#define MyAppName "Voice Chat Client"
#define MyAppVersion "1.0"
#define MyAppPublisher "Dmitriy"
#define MyAppExeName "VoiceClient.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-1234-567890ABCDEF}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={userappdata}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=
OutputDir=.\installer_output
OutputBaseFilename=VoiceClient_Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
; Копируем всё из папки сборки клиента
; ВАЖНО: Укажи правильный путь к папке build, где лежит готовый Release!
Source: "C:\Program Files (x86)\dev\voice_chat\client\build\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Если у тебя файлы лежат просто в build, а не в build\Release, убери \Release из пути выше.

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent