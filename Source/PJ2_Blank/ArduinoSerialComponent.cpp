// [학습] 게임 스레드에서 대기 데이터를 제한량만 읽는다. 메시지를 기다리느라 프레임을 멈추지 않도록 읽기 타임아웃을 설정한다.
#include "ArduinoSerialComponent.h"
// [학습] 이 블록의 Windows API는 Windows 빌드에서만 사용한다.
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

// [학습] Tick을 켜서 매 프레임 수신 대기 데이터를 확인할 수 있게 한다.
UArduinoSerialComponent::UArduinoSerialComponent() { PrimaryComponentTick.bCanEverTick = true; }

// [학습] 상태가 실제로 바뀔 때만 이벤트를 보낸다. 매 프레임 같은 상태 로그가 쌓이는 것을 피한다.
void UArduinoSerialComponent::SetState(ESerialPortState State)
{
    if (PortState != State) { PortState = State; OnPortStateChanged.Broadcast(State); }
}

// [학습] 실패 경로를 한곳에 모아 핸들·부분 줄을 정리하고 오류 상태와 진단 문구를 전달한다.
bool UArduinoSerialComponent::Fail(const FString& Message)
{
    ClosePort(); LastError = Message; SetState(ESerialPortState::Error);
    UE_LOG(LogTemp, Warning, TEXT("Arduino serial: %s"), *LastError);
    return false;
}

// [학습] 여러 번 닫아도 안전하게 처리한다. 재연결에 예전 부분 메시지가 남지 않도록 버퍼도 초기화한다.
void UArduinoSerialComponent::ClosePort()
{
// [학습] 이 블록의 Windows API는 Windows 빌드에서만 사용한다.
#if PLATFORM_WINDOWS
    if (SerialHandle) { CloseHandle(SerialHandle); }
#endif
    SerialHandle = nullptr; LineBuffer.Reset(); SetState(ESerialPortState::Closed);
}

// [학습] 중복 열기는 거부한다. 실패한 초기화 중에 얻은 핸들도 Fail을 통해 닫는다.
bool UArduinoSerialComponent::OpenPort()
{
    if (SerialHandle) { return false; }
    ClosePort(); LastError.Reset();
    if (PortName.IsEmpty() || BaudRate <= 0) { return Fail(TEXT("Invalid port name or baud rate")); }
// [학습] 이 블록의 Windows API는 Windows 빌드에서만 사용한다.
#if PLATFORM_WINDOWS
    // [학습] COM10 이상도 다룰 수 있는 Windows 장치 경로 형식이다. 문자열의 역슬래시는 C++에서 두 번 적는다.
    const FString Path = TEXT("\\\\.\\") + PortName;
    // [학습] 읽기·쓰기 권한으로 장치를 독점해서 연다. 시리얼 모니터가 점유 중이면 실패할 수 있다.
    HANDLE Handle = CreateFileW(*Path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (Handle == INVALID_HANDLE_VALUE)
    { return Fail(FString::Printf(TEXT("Cannot open %s (Windows error %lu)"), *PortName, GetLastError())); }
    SerialHandle = Handle;
    // [학습] 운영체제의 시리얼 설정 구조체. 현재 설정을 읽은 뒤 필요한 8N1과 흐름 제어 옵션을 명시한다.
    DCB Config = {}; Config.DCBlength = sizeof(Config);
    if (!GetCommState(Handle, &Config)) { return Fail(TEXT("GetCommState failed")); }
    Config.BaudRate = BaudRate; Config.ByteSize = 8; Config.Parity = NOPARITY; Config.StopBits = ONESTOPBIT;
    Config.fBinary = true; Config.fParity = false;
    Config.fOutxCtsFlow = false; Config.fOutxDsrFlow = false; Config.fDsrSensitivity = false;
    Config.fDtrControl = DTR_CONTROL_ENABLE; Config.fRtsControl = RTS_CONTROL_ENABLE;
    Config.fOutX = false; Config.fInX = false; Config.fAbortOnError = false;
    if (!SetCommState(Handle, &Config)) { return Fail(TEXT("SetCommState failed")); }
    // [학습] MAXDWORD + 총 읽기 타임아웃 0은 지금 대기 중인 바이트만 즉시 반환하도록 하는 설정이다.
    COMMTIMEOUTS Timeouts = {}; Timeouts.ReadIntervalTimeout = MAXDWORD;
    Timeouts.WriteTotalTimeoutConstant = 100;
    // [학습] 시작 전 운영체제 버퍼를 비운다. 포트를 열 때 보드가 재시작할 수 있어 첫 입력은 나중에 도착할 수 있다.
    if (!SetCommTimeouts(Handle, &Timeouts) || !PurgeComm(Handle, PURGE_RXCLEAR | PURGE_TXCLEAR))
    { return Fail(TEXT("Serial initialization failed")); }
    SetState(ESerialPortState::Open); return true;
#else
    return Fail(TEXT("Serial input is supported on Windows only"));
#endif
}

// [학습] 상태 확인 → 제한된 ReadFile → 줄 조립 순서. Tick 하나에서 무한히 읽지 않는다.
void UArduinoSerialComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
// [학습] 이 블록의 Windows API는 Windows 빌드에서만 사용한다.
#if PLATFORM_WINDOWS
    // [학습] 한 프레임의 총 수신 바이트 예산. 큰 입력이 와도 이 코드가 프레임을 끝없이 점유하지 않게 한다.
    int32 Budget = 1024;
    while (SerialHandle && Budget > 0)
    {
        // [학습] ClearCommError는 통신 오류와 운영체제 수신 큐의 바이트 수를 함께 확인한다.
        DWORD Errors = 0; COMSTAT Status = {};
        if (!ClearCommError(SerialHandle, &Errors, &Status) || Errors != 0)
        { Fail(TEXT("Serial status error; check USB and retry")); break; }
        if (Status.cbInQue == 0) { break; }
        // [학습] 한 번 읽는 청크는 최대 256바이트다. 읽기 결과가 메시지 한 줄과 일치한다고 가정하지 않는다.
        uint8 Bytes[256]; DWORD Read = 0;
        const DWORD Count = FMath::Min<DWORD>(FMath::Min<DWORD>(Status.cbInQue, 256), Budget);
        if (!ReadFile(SerialHandle, Bytes, Count, &Read, nullptr))
        { Fail(TEXT("Serial read failed; check USB and retry")); break; }
        if (Read == 0) { break; }
        // [학습] 요청한 크기가 아니라 실제 읽은 바이트 수로 예산을 차감한다.
        Budget -= Read;
        // [학습] 완성된 줄만 이벤트로 전달한다. 이벤트 안에서 재연결하면 버퍼의 세대 검사로 옛 배치를 중단한다.
        LineBuffer.Consume(Bytes, Read, [this](const FString& Line) { OnLineReceived.Broadcast(Line); });
    }
#endif
}

// [학습] UTF-8 바이트로 쓰고 실제 쓴 길이를 확인한다. 동기 쓰기이며 최대 100ms 타임아웃이라 Tick의 반복 송신용으로 쓰지 않는다.
bool UArduinoSerialComponent::SendText(const FString& Text)
{
    if (!SerialHandle) { return false; }
// [학습] 이 블록의 Windows API는 Windows 빌드에서만 사용한다.
#if PLATFORM_WINDOWS
    FTCHARToUTF8 Encoded(*Text); DWORD Written = 0;
    if (!WriteFile(SerialHandle, Encoded.Get(), Encoded.Length(), &Written, nullptr) || Written != static_cast<DWORD>(Encoded.Length()))
    { return Fail(TEXT("Serial write failed or incomplete")); }
    return true;
#else
    return false;
#endif
}

// [학습] Play 종료·레벨 종료 때 포트를 해제해야 다음 실행과 다른 프로그램이 다시 사용할 수 있다.
void UArduinoSerialComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{ ClosePort(); Super::EndPlay(EndPlayReason); }
