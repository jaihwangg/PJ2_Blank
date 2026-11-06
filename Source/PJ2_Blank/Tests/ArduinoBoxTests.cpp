// [학습] 테스트용 World에 실제 상자 Actor를 만들고 위치 이동과 Reset 후 정지를 검사한다. USB 조작 검증을 대신하지는 않는다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "../ArduinoJoystickBox.h"
#include "../ArduinoJoystickComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArduinoBoxTest, "PJ2.Arduino.BoxMovement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArduinoBoxTest::RunTest(const FString& Parameters)
{
    // [학습] CreateWorld가 이미 초기화하므로 InitializeNewWorld를 다시 부르지 않는다.
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AArduinoJoystickBox* Box = World->SpawnActor<AArduinoJoystickBox>();
    Box->SetActorLocation(FVector(0, 0, 50));
    Box->Joystick->HandleSerialLine(TEXT("JOY,1023,512"));
    Box->Tick(1.f);
    TestTrue(TEXT("real actor moves 300 units"), Box->GetActorLocation().Equals(FVector(300, 0, 50), .01));
    Box->Joystick->ResetInput();
    Box->Tick(1.f);
    TestTrue(TEXT("reset stops real actor"), Box->GetActorLocation().Equals(FVector(300, 0, 50), .01));
    // [학습] 테스트가 만든 World는 테스트가 끝날 때 정리한다.
    World->DestroyWorld(false);
    return true;
}
#endif
