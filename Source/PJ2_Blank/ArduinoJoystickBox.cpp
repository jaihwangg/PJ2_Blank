// [학습] 통신·입력 컴포넌트를 가진 상자를 만들고 보정된 두 축을 월드 X/Y 위치 이동에 사용한다.
#include "ArduinoJoystickBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

// [학습] 생성자에서 기본 컴포넌트와 메쉬를 구성한다. 맵에 배치한 Blueprint 인스턴스도 이를 상속한다.
AArduinoJoystickBox::AArduinoJoystickBox()
{
    PrimaryActorTick.bCanEverTick = true;
    BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
    SetRootComponent(BoxMesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) { BoxMesh->SetStaticMesh(Cube.Object); }
    // [학습] 위치 이동이 가능하도록 Movable로 둔다. 현재 예제는 물리·충돌 기능을 사용하지 않는다.
    BoxMesh->SetMobility(EComponentMobility::Movable);
    BoxMesh->SetSimulatePhysics(false);
    BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // [학습] 기능을 같은 Actor에 붙여 서로 찾고 이벤트를 연결할 수 있게 한다.
    Serial = CreateDefaultSubobject<UArduinoSerialComponent>(TEXT("Serial"));
    Joystick = CreateDefaultSubobject<UArduinoJoystickComponent>(TEXT("Joystick"));
}

// [학습] 부모 BeginPlay가 컴포넌트 시작을 처리한 뒤, 진단과 R 키를 연결하고 포트를 연다.
void AArduinoJoystickBox::BeginPlay()
{
    Super::BeginPlay();
    // [학습] Serial → Joystick → Actor 순서에서 Actor가 마지막에 이동하도록 한다.
    AddTickPrerequisiteComponent(Joystick);
    Joystick->OnInputStateChanged.AddDynamic(this, &AArduinoJoystickBox::LogInputState);
    Serial->OnPortStateChanged.AddDynamic(this, &AArduinoJoystickBox::LogPortState);
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        EnableInput(PC);
        // [학습] Player 0의 R 키를 재연결 함수에 연결한다. 게임 화면이 키 입력 포커스를 받아야 한다.
        InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AArduinoJoystickBox::RetryConnection);
    }
    if (bAutoOpenPort) { Serial->OpenPort(); }
}

// [학습] 이전 연결과 입력을 먼저 지운 다음 새 포트를 연다. 새 데이터가 오기 전에는 정지한다.
void AArduinoJoystickBox::RetryConnection()
{ Serial->ClosePort(); Joystick->ResetInput(); Serial->OpenPort(); }

// [학습] 상태가 변할 때 남기는 로그다. 모든 입력 샘플을 매번 로그 파일에 쓰지는 않는다.
void AArduinoJoystickBox::LogInputState(EJoystickInputState State)
{ UE_LOG(LogTemp, Display, TEXT("Joystick input: %s"), *UEnum::GetValueAsString(State)); }

void AArduinoJoystickBox::LogPortState(ESerialPortState State)
{ UE_LOG(LogTemp, Display, TEXT("Joystick port: %s %s"), *UEnum::GetValueAsString(State), *Serial->LastError); }

// [학습] DeltaSeconds는 이번 프레임까지 흐른 초 단위 시간이다. FPS에 관계없이 비슷한 초당 속도를 만들 때 필요하다.
void AArduinoJoystickBox::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const FVector2D Axis = Joystick->GetCurrentAxis(); // Recheck monotonic timeout before moving, including after pause.
    // [학습] 현재 위치에 월드 이동량을 더한다. Z가 0이라 높이는 유지되고, sweep=false라 장애물 충돌 검사는 하지 않는다.
    AddActorWorldOffset(ArduinoJoystick::CalculateMovement(Axis, MoveSpeed, DeltaSeconds), false);
    if (bShowDiagnostics && GEngine)
    {
        const FString Text = FString::Printf(TEXT("%s @ %d | %s | %s\nRaw %d, %d | Axis %.2f, %.2f | R: retry\n%s %s"),
            *Serial->PortName, Serial->BaudRate, *UEnum::GetValueAsString(Serial->PortState),
            *UEnum::GetValueAsString(Joystick->InputState), Joystick->RawInput.X, Joystick->RawInput.Y,
            Axis.X, Axis.Y, *Serial->LastError, *Joystick->ConfigurationError);
        // [학습] Actor마다 같은 키의 화면 메시지를 갱신한다. Raw는 원본, Axis는 보정된 값이다.
        GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::Cyan, Text);
    }
}
