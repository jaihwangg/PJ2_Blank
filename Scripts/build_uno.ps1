# [학습] 설치된 Arduino AVR 코어와 컴파일러를 직접 쓰는 오프라인 빌드 보조 도구다. 보드 업로드는 하지 않는다.
param(
    [string]$ArduinoData = "$env:LOCALAPPDATA/Arduino15",
    [string]$CoreVersion = '1.8.8',
    [string]$CompilerVersion = '7.3.0-atmel3.6.1-arduino7'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$core = Join-Path $ArduinoData "packages/arduino/hardware/avr/$CoreVersion/cores/arduino"
$variant = Join-Path $ArduinoData "packages/arduino/hardware/avr/$CoreVersion/variants/standard"
$tools = Join-Path $ArduinoData "packages/arduino/tools/avr-gcc/$CompilerVersion/bin"
$out = Join-Path $projectRoot 'Intermediate/Arduino/JoystickBox'
New-Item -ItemType Directory -Force $out | Out-Null
# [학습] Uno의 MCU·클럭·헤더 경로와 공통 컴파일 옵션을 맞춘다. 도구 버전은 위 매개변수로 지정한다.
$common = @('-c','-g','-Os','-ffunction-sections','-fdata-sections','-flto','-mmcu=atmega328p','-DF_CPU=16000000L','-DARDUINO=10819','-DARDUINO_AVR_UNO','-DARDUINO_ARCH_AVR',"-I$core","-I$variant")
$objects = @()
# [학습] 스케치가 사용하는 Arduino 기본 코어의 C/C++/어셈블리 파일을 컴파일한다.
foreach ($source in Get-ChildItem -LiteralPath $core -File | Where-Object Extension -In '.c','.cpp','.S') {
    $object = Join-Path $out ($source.Name + '.o')
    $compiler = Join-Path $tools 'avr-gcc.exe'
    $flags = @('-std=gnu11')
    if ($source.Extension -eq '.cpp') {
        $compiler = Join-Path $tools 'avr-g++.exe'
        $flags = @('-std=gnu++11','-fpermissive','-fno-exceptions','-fno-threadsafe-statics')
    } elseif ($source.Extension -eq '.S') { $flags = @('-x','assembler-with-cpp') }
    & $compiler @common @flags $source.FullName -o $object
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $($source.Name)" }
    $objects += $object
}
# [학습] 스케치 자체도 C++로 컴파일한다. 이 예제는 추가 전처리 함수 선언 생성에 의존하지 않는다.
$sketchObject = Join-Path $out 'JoystickBox.ino.o'
& (Join-Path $tools 'avr-g++.exe') @common -std=gnu++11 -fno-exceptions -fno-threadsafe-statics -x c++ -include Arduino.h (Join-Path $projectRoot 'Arduino/JoystickBox/JoystickBox.ino') -o $sketchObject
if ($LASTEXITCODE -ne 0) { throw 'Sketch compilation failed' }
# [학습] 코어와 스케치의 목적 파일을 링크해 실행 이미지를 만든 뒤 업로드 형식인 HEX로 변환한다.
$elf = Join-Path $out 'JoystickBox.elf'
& (Join-Path $tools 'avr-gcc.exe') -Os -flto -fuse-linker-plugin '-Wl,--gc-sections' -mmcu=atmega328p -o $elf $sketchObject @objects -lm
if ($LASTEXITCODE -ne 0) { throw 'Firmware link failed' }
& (Join-Path $tools 'avr-objcopy.exe') -O ihex -R .eeprom $elf (Join-Path $out 'JoystickBox.hex')
if ($LASTEXITCODE -ne 0) { throw 'HEX generation failed' }
& (Join-Path $tools 'avr-size.exe') -C --mcu=atmega328p $elf
Write-Output "UNO_BUILD_OK $elf"
