// [학습] BP_JoystickBox의 부모 C++ Actor다. Blueprint 그래프에 이동 노드가 없어도 부모의 Tick에서 이동한다.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArduinoJoystickComponent.h"
#include "ArduinoJoystickBox.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class PJ2_BLANK_API AArduinoJoystickBox : public AActor
{
    GENERATED_BODY()
public:
    AArduinoJoystickBox();
    // [학습] 화면에 보이는 상자이며 Actor의 루트다. 물리 시뮬레이션 대신 위치 이동으로 제어한다.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Joystick Box") TObjectPtr<UStaticMeshComponent> BoxMesh;
    // [학습] 통신 담당 컴포넌트. 포트 이름과 속도는 에디터에서 설정한다.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Joystick Box") TObjectPtr<UArduinoSerialComponent> Serial;
    // [학습] 입력 보정·만료 담당 컴포넌트. Serial이 받은 줄을 해석한다.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Joystick Box") TObjectPtr<UArduinoJoystickComponent> Joystick;
    // [학습] 초당 이동 거리다. 프레임당 거리가 아니며 Tick에서 DeltaSeconds를 곱한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick Box", meta=(ClampMin="0")) float MoveSpeed = 300;
    // [학습] Play 시작 시 자동으로 연결한다. 재연결은 R 또는 RetryConnection 호출로 수동 수행한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick Box") bool bAutoOpenPort = true;
    // [학습] Raw·Axis·포트·입력 상태를 게임 화면에 표시할지 정한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joystick Box") bool bShowDiagnostics = true;
    UFUNCTION(BlueprintCallable, Category="Joystick Box") void RetryConnection();
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
private:
    UFUNCTION() void LogInputState(EJoystickInputState State);
    UFUNCTION() void LogPortState(ESerialPortState State);
};
