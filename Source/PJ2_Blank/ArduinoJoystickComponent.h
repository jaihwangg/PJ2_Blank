// [학습] 시리얼 컴포넌트의 줄을 조이스틱 상태로 바꾼다. 장치 연결과 게임 물체 이동 사이의 입력 담당자다.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArduinoSerialComponent.h"
#include "ArduinoJoystickComponent.generated.h"

UENUM(BlueprintType)
// [학습] 닫힘 → 첫 입력 대기 → 유효 입력 수신 → 입력 만료를 구분한다. 포트 상태와 별개다.
enum class EJoystickInputState : uint8 { Disconnected, WaitingForInput, Receiving, Stale };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FArduinoInputStateChanged, EJoystickInputState, State);

UCLASS(ClassGroup=(Arduino), meta=(BlueprintSpawnableComponent))
class PJ2_BLANK_API UArduinoJoystickComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UArduinoJoystickComponent();
    // [학습] 두 축의 보정된 값. 이동 직전에는 GetCurrentAxis를 호출해 만료 여부까지 다시 확인한다.
    UPROPERTY(BlueprintReadOnly, Category="Joystick") FVector2D Axis = FVector2D::ZeroVector;
    // [학습] 가장 최근의 유효 JOY 줄에서 받은 원본 정수 두 개. 중심·끝값 보정에 참고한다.
    UPROPERTY(BlueprintReadOnly, Category="Joystick") FIntPoint RawInput = FIntPoint::ZeroValue;
    UPROPERTY(BlueprintReadOnly, Category="Joystick") EJoystickInputState InputState = EJoystickInputState::Disconnected;
    UPROPERTY(BlueprintReadOnly, Category="Joystick") FString ConfigurationError;
    UPROPERTY(BlueprintAssignable, Category="Joystick") FArduinoInputStateChanged OnInputStateChanged;
    // [학습] 축마다 Min < Center < Max가 되어야 한다. 실제 부품의 중심이 기본값 512와 다를 수 있다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration") int32 MinX = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration") int32 CenterX = 512;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration") int32 MaxX = 1023;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration") int32 MinY = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration") int32 CenterY = 512;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration") int32 MaxY = 1023;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration") bool bInvertX = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration") bool bInvertY = false;
    // [학습] 각 축의 정규화 값 중 중심 주변을 0으로 만드는 비율. 0.10은 기본 중심 데드존이다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick|Calibration", meta=(ClampMin="0", ClampMax="0.99")) float DeadZone = .1f;
    // [학습] 마지막 유효 메시지 이후 이 시간이 지나면 입력을 0으로 만든다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick", meta=(ClampMin="0.01")) float InputTimeoutSeconds = .5f;
    UFUNCTION(BlueprintCallable, Category="Joystick") FVector2D GetCurrentAxis();
    UFUNCTION(BlueprintCallable, Category="Joystick") void ResetInput();
    UFUNCTION() void HandleSerialLine(const FString& Line);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UPROPERTY() TObjectPtr<UArduinoSerialComponent> Serial;
    // [학습] FPlatformTime의 경과 시각을 저장한다. 게임 시간 배율에 기대지 않고 오래된 입력을 판단한다.
    double LastValidTime = 0;
    // [학습] 실제 첫 입력을 받은 적이 있는지 구분해 WaitingForInput과 Stale을 혼동하지 않는다.
    bool bHasInput = false;
    void SetState(EJoystickInputState State);
    UFUNCTION() void HandlePortState(ESerialPortState State);
};
