// [학습] 실제 조이스틱·시리얼 컴포넌트의 상태 변화를 검사한다. 없는 포트를 여는 실패는 의도한 테스트다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ArduinoJoystickComponent.h"
#include "../ArduinoSerialComponent.h"
#include "HAL/PlatformProcess.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArduinoComponentTest, "PJ2.Arduino.ComponentInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArduinoComponentTest::RunTest(const FString& Parameters)
{
    UArduinoJoystickComponent* Joystick = NewObject<UArduinoJoystickComponent>();
    Joystick->HandleSerialLine(TEXT("JOY,1023,512"));
    TestEqual(TEXT("receiving"), Joystick->InputState, EJoystickInputState::Receiving);
    TestTrue(TEXT("positive X"), Joystick->GetCurrentAxis().Equals(FVector2D(1, 0)));
    Joystick->HandleSerialLine(TEXT("JOY,broken,512"));
    TestTrue(TEXT("bad line preserves last valid input"), Joystick->GetCurrentAxis().Equals(FVector2D(1, 0)));
    // [학습] 시험에서만 짧은 만료 시간을 사용해, 무효 메시지가 유효 시각을 연장하지 않는지 확인한다.
    Joystick->InputTimeoutSeconds = .01f;
    FPlatformProcess::Sleep(.025f);
    Joystick->HandleSerialLine(TEXT("JOY,invalid,512"));
    TestTrue(TEXT("invalid line cannot extend timeout"), Joystick->GetCurrentAxis().IsNearlyZero());
    TestEqual(TEXT("stale state"), Joystick->InputState, EJoystickInputState::Stale);
    Joystick->InputTimeoutSeconds = .5f;
    Joystick->HandleSerialLine(TEXT("JOY,1023,512"));
    TestEqual(TEXT("fresh line recovers"), Joystick->InputState, EJoystickInputState::Receiving);
    Joystick->ResetInput();
    TestTrue(TEXT("close neutralizes"), Joystick->GetCurrentAxis().IsNearlyZero());
    Joystick->CenterX = Joystick->MinX;
    Joystick->HandleSerialLine(TEXT("JOY,1023,1023"));
    TestTrue(TEXT("invalid calibration neutralizes both axes"), Joystick->GetCurrentAxis().IsNearlyZero());
    UArduinoSerialComponent* Serial = NewObject<UArduinoSerialComponent>();
    // [학습] 실물 장치의 특정 COM 번호에 의존하지 않고 열기 실패와 오류 상태를 검증한다.
    Serial->PortName = TEXT("INVALID_JOYSTICK_TEST_PORT");
    TestFalse(TEXT("missing port returns failure"), Serial->OpenPort());
    TestEqual(TEXT("failed state"), Serial->PortState, ESerialPortState::Error);
    TestFalse(TEXT("error diagnosis retained"), Serial->LastError.IsEmpty());
    TestFalse(TEXT("closed send fails safely"), Serial->SendText(TEXT("test")));
    Serial->ClosePort(); Serial->ClosePort();
    TestEqual(TEXT("idempotent close"), Serial->PortState, ESerialPortState::Closed);
    return true;
}
#endif
