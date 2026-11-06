// [학습] 수신 이벤트를 연결하고 원본값·유효 수신 시각을 기억하며, 현재 프레임에 쓸 축을 제공한다.
#include "ArduinoJoystickComponent.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/Actor.h"

UArduinoJoystickComponent::UArduinoJoystickComponent() { PrimaryComponentTick.bCanEverTick = true; }

// [학습] 같은 Actor에서 Serial을 찾아 이벤트를 연결한다. 실제 포트 열기는 상자 BeginPlay가 담당한다.
void UArduinoJoystickComponent::BeginPlay()
{
    Super::BeginPlay();
    Serial = GetOwner()->FindComponentByClass<UArduinoSerialComponent>();
    if (Serial)
    {
        Serial->OnLineReceived.AddDynamic(this, &UArduinoJoystickComponent::HandleSerialLine);
        Serial->OnPortStateChanged.AddDynamic(this, &UArduinoJoystickComponent::HandlePortState);
        // [학습] 이번 프레임의 시리얼 읽기가 끝난 뒤 조이스틱 갱신이 실행되도록 순서를 지정한다.
        AddTickPrerequisiteComponent(Serial);
        HandlePortState(Serial->PortState);
    }
    else { ConfigurationError = TEXT("Owner needs an ArduinoSerialComponent"); }
}

void UArduinoJoystickComponent::SetState(EJoystickInputState State)
{ if (InputState != State) { InputState = State; OnInputStateChanged.Broadcast(State); } }

// [학습] 닫기·재시도 때 원본·축·시각을 모두 지워 예전 방향으로 움직이지 않게 한다.
void UArduinoJoystickComponent::ResetInput()
{
    Axis = FVector2D::ZeroVector; RawInput = FIntPoint::ZeroValue;
    LastValidTime = 0; bHasInput = false; SetState(EJoystickInputState::Disconnected);
}

// [학습] 포트가 열리면 첫 입력 대기, 그 밖의 포트 상태에서는 입력을 초기화한다.
void UArduinoJoystickComponent::HandlePortState(ESerialPortState State)
{
    ResetInput();
    if (State == ESerialPortState::Open) { SetState(EJoystickInputState::WaitingForInput); }
}

// [학습] 파싱에 실패한 줄은 원본값뿐 아니라 마지막 유효 시각도 갱신하지 않는다.
void UArduinoJoystickComponent::HandleSerialLine(const FString& Line)
{
    FIntPoint Raw;
    if (!ArduinoJoystick::ParseLine(Line, Raw)) { return; }
    RawInput = Raw; LastValidTime = FPlatformTime::Seconds(); bHasInput = true;
    // [학습] 현재 순서는 상태 알림 후 축 계산이다. 이 이벤트 콜백에서는 Axis 속성보다 GetCurrentAxis로 최신값을 읽는다.
    SetState(EJoystickInputState::Receiving); GetCurrentAxis();
}

// [학습] 이동하는 쪽이 호출할 때마다 만료와 설정을 확인한다. 반환 전 기본값은 정지인 0이다.
FVector2D UArduinoJoystickComponent::GetCurrentAxis()
{
    Axis = FVector2D::ZeroVector; ConfigurationError.Reset();
    if (!bHasInput) { return Axis; }
    // [학습] 연결 오류를 즉시 알아채지 못해도 유효 입력이 오래 끊기면 정지한다.
    if (ArduinoJoystick::IsStale(LastValidTime, FPlatformTime::Seconds(), InputTimeoutSeconds))
    { SetState(EJoystickInputState::Stale); return Axis; }
    float X = 0, Y = 0;
    // [학습] 한 축이라도 보정 설정이 잘못되면 두 축 모두 0을 반환하고 오류 설명을 표시한다.
    if (!ArduinoJoystick::NormalizeAxis(RawInput.X, MinX, CenterX, MaxX, DeadZone, bInvertX, X)
        || !ArduinoJoystick::NormalizeAxis(RawInput.Y, MinY, CenterY, MaxY, DeadZone, bInvertY, Y))
    {
        ConfigurationError = TEXT("Require 0 <= Min < Center < Max <= 1023 and 0 <= DeadZone < 1");
        return Axis;
    }
    Axis = FVector2D(X, Y); return Axis;
}

// [학습] 상자가 읽지 않는 프레임에도 입력 상태가 갱신되도록 매 프레임 만료를 확인한다.
void UArduinoJoystickComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{ Super::TickComponent(DeltaTime, TickType, ThisTickFunction); GetCurrentAxis(); }

// [학습] 자신이 등록한 이벤트 연결을 해제하고 상태를 초기화한다.
void UArduinoJoystickComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Serial)
    {
        Serial->OnLineReceived.RemoveDynamic(this, &UArduinoJoystickComponent::HandleSerialLine);
        Serial->OnPortStateChanged.RemoveDynamic(this, &UArduinoJoystickComponent::HandlePortState);
    }
    ResetInput(); Super::EndPlay(EndPlayReason);
}
