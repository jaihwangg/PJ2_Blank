// [학습] Windows COM 장치와 통신하는 컴포넌트의 설정·상태·이벤트를 선언한다. 조이스틱 데이터의 의미는 해석하지 않는다.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArduinoJoystickProtocol.h"
#include "ArduinoSerialComponent.generated.h"

UENUM(BlueprintType)
// [학습] 포트 Open과 정상 조이스틱 수신은 별개다. 장치가 열려도 메시지가 없을 수 있다.
enum class ESerialPortState : uint8 { Closed, Open, Error };
// [학습] 여러 수신자가 연결할 수 있는 줄 알림이다. Blueprint에서도 이벤트를 연결할 수 있다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FArduinoLineReceived, const FString&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FArduinoPortStateChanged, ESerialPortState, State);

UCLASS(ClassGroup=(Arduino), meta=(BlueprintSpawnableComponent))
class PJ2_BLANK_API UArduinoSerialComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UArduinoSerialComponent();
    // [학습] PC마다 COM 번호가 달라질 수 있다. 실제 연결 장치에 맞춰 에디터에서 바꾼다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Serial") FString PortName = TEXT("COM3");
    // [학습] Uno의 Serial.begin 값과 같아야 한다. 기존 인스턴스의 저장값도 Details에서 확인한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Serial", meta=(ClampMin="1")) int32 BaudRate = 115200;
    UPROPERTY(BlueprintReadOnly, Category="Serial") ESerialPortState PortState = ESerialPortState::Closed;
    UPROPERTY(BlueprintReadOnly, Category="Serial") FString LastError;
    // [학습] 줄 경계까지만 처리한 데이터다. 올바른 JOY 형식인지 여부는 조이스틱 쪽에서 검사한다.
    UPROPERTY(BlueprintAssignable, Category="Serial") FArduinoLineReceived OnLineReceived;
    UPROPERTY(BlueprintAssignable, Category="Serial") FArduinoPortStateChanged OnPortStateChanged;
    UFUNCTION(BlueprintCallable, Category="Serial") bool OpenPort();
    UFUNCTION(BlueprintCallable, Category="Serial") void ClosePort();
    // [학습] 기존 API 호환용 송신 함수다. 이번 상자 이동은 호출하지 않으며, 출력 명령 프로토콜도 아직 없다.
    UFUNCTION(BlueprintCallable, Category="Serial") bool SendText(const FString& Text);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    // [학습] 운영체제가 돌려준 장치 이용 핸들. nullptr이면 보관 중인 열린 장치가 없다.
    void* SerialHandle = nullptr;
    FArduinoLineBuffer LineBuffer;
    void SetState(ESerialPortState State);
    bool Fail(const FString& Message);
};
