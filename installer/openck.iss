[Setup]
AppName=OpenCK
AppVersion=1.0.0
AppPublisher=OpenCK Contributors
DefaultDirName={autopf}\OpenCK
DefaultGroupName=OpenCK
OutputDir=installer\output
OutputBaseFilename=OpenCK-1.0.0-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
LicenseFile=..\LICENSE
PrivilegesRequired=admin

[Files]
Source: "..\build\bin\Release\openck.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\Release\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\Release\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion
Source: "..\build\bin\Release\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion
Source: "..\build\bin\Release\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion
Source: "..\build\bin\Release\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion
Source: "..\data\editor.ini"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\OpenCK"; Filename: "{app}\openck.exe"
Name: "{group}\Uninstall OpenCK"; Filename: "{uninstallexe}"
Name: "{autodesktop}\OpenCK"; Filename: "{app}\openck.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"

[Run]
Filename: "{app}\openck.exe"; Description: "Launch OpenCK"; Flags: nowait postinstall skipifsilent
