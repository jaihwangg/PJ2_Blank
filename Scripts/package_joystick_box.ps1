# [학습] Windows 배포용 빌드·Cook·Stage·Archive 도구다. 사용자는 현재 에디터 학습을 먼저 하기로 했으므로 실행할 필요 없다.
param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [switch]$NoShaderWorker
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
# [학습] 엔진 설치 폴더 대신 프로젝트 Saved 아래에 자동화 로그를 저장한다.
$env:uebp_EngineSavedFolder = Join-Path $projectRoot 'Saved/AutomationTool'
$dotnet = Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/DotNet/10.0/win-x64/dotnet.exe'
$uat = Join-Path $EngineRoot 'Engine/Binaries/DotNET/AutomationTool/AutomationTool.dll'
$cookOptions = "-NullRHI -ShaderWorkingDir=$projectRoot/Intermediate/ShaderWork"
# [학습] 별도 셰이더 작업자 실행이 제한된 환경에서는 프로세스 내부 컴파일을 선택한다. 더 느릴 수 있다.
if ($NoShaderWorker) { $cookOptions += ' -noshaderworker' }
# [학습] 소스 빌드와 에셋 Cook 후 패키지 구성까지 실행하는 긴 작업이다. 에디터 Play와는 별도 단계다.
& $dotnet $uat BuildCookRun "-project=$projectRoot/PJ2_Blank.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook '-map=/Game/Maps/L_JoystickBoxTest' -stage -pak -archive "-archivedirectory=$projectRoot/Saved/Packages/JoystickBox" -unattended -utf8output '-ubtargs=-NoUBA -WaitMutex' -skipbuildeditor -nocompile -nocompileuat "-AdditionalCookerOptions=$cookOptions"
if ($LASTEXITCODE -ne 0) { throw "Packaging failed with exit code $LASTEXITCODE" }
Write-Output "Run: $projectRoot/Saved/Packages/JoystickBox/Windows/PJ2_Blank.exe /Game/Maps/L_JoystickBoxTest"
