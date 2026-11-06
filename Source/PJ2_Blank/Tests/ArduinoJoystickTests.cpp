// [학습] 하드웨어 없이 실제 프로토콜·보정 함수를 검증한다. 기대값은 독립적으로 정한 숫자로 비교한다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ArduinoJoystickProtocol.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArduinoProtocolTest, "PJ2.Arduino.Protocol", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
// [학습] 문장 형식, 부분 도착, 다중 줄, 길이 초과와 재연결 중 옛 데이터 폐기를 확인한다.
bool FArduinoProtocolTest::RunTest(const FString& Parameters)
{
    FIntPoint Value(-1, -1);
    TestTrue(TEXT("valid sample"), ArduinoJoystick::ParseLine(TEXT("JOY,512,508"), Value));
    TestEqual(TEXT("X"), Value.X, 512);
    TestEqual(TEXT("Y"), Value.Y, 508);
    for (const TCHAR* Bad : {TEXT("JOY,,1"), TEXT("JOY,1,2,3"), TEXT("JOY,-1,2"), TEXT("JOY,1024,0"), TEXT("JOY,1x,0"), TEXT("JOY, 1,0"), TEXT("JOY,+1,0"), TEXT("JOY,1.0,0"), TEXT("joy,1,0"), TEXT("")})
    {
        TestFalse(Bad, ArduinoJoystick::ParseLine(Bad, Value));
    }
    FArduinoLineBuffer Buffer;
    FString Line;
    TArray<FString> Lines;
    const auto Feed = [&](const FString& Chunk)
    {
        for (TCHAR C : Chunk) { if (Buffer.Push(static_cast<uint8>(C), Line)) { Lines.Add(Line); } }
    };
    Feed(TEXT("JOY,5"));
    TestEqual(TEXT("partial line waits"), Lines.Num(), 0);
    Feed(TEXT("12,508\r\nJOY,0,1023\n"));
    TestEqual(TEXT("multiple lines"), Lines.Num(), 2);
    TestEqual(TEXT("CR removed"), Lines[0], FString(TEXT("JOY,512,508")));
    Feed(FString::ChrN(65, TEXT('x')) + TEXT("JOY,1,2\nJOY,3,4\n"));
    TestEqual(TEXT("oversized line discarded until LF"), Lines.Num(), 3);
    TestEqual(TEXT("resynchronized"), Lines.Last(), FString(TEXT("JOY,3,4")));
    Feed(FString::ChrN(64, TEXT('x')) + TEXT("\n"));
    TestEqual(TEXT("64 bytes accepted"), Lines.Num(), 4);
    Feed(TEXT("old")); Buffer.Reset(); Feed(TEXT("JOY,7,8\n"));
    TestEqual(TEXT("reconnect resets partial buffer"), Lines.Last(), FString(TEXT("JOY,7,8")));
    // [학습] 콜백에서 Reset하는 상황을 재현한다. 옛 배치의 두 번째 줄과 부분 글자가 새 연결에 섞이면 실패한다.
    FArduinoLineBuffer ReconnectBuffer;
    TArray<FString> Delivered;
    const ANSICHAR* Batch = "JOY,1,2\nJOY,1000,0\nold";
    ReconnectBuffer.Consume(reinterpret_cast<const uint8*>(Batch), FCStringAnsi::Strlen(Batch), [&](const FString& Sample)
    {
        Delivered.Add(Sample);
        ReconnectBuffer.Reset(); // The real listener can synchronously close and reopen the port.
    });
    TestEqual(TEXT("reconnect during delivery discards old batch"), Delivered.Num(), 1);
    const ANSICHAR* Fresh = "JOY,3,4\n";
    ReconnectBuffer.Consume(reinterpret_cast<const uint8*>(Fresh), FCStringAnsi::Strlen(Fresh), [&](const FString& Sample) { Delivered.Add(Sample); });
    TestEqual(TEXT("fresh connection has no old partial bytes"), Delivered.Last(), FString(TEXT("JOY,3,4")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArduinoAxisTest, "PJ2.Arduino.CalibrationAndMovement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
// [학습] 중심·끝값·반전·설정 오류, 시간 경계, FPS별 이동 거리와 대각선 속도를 확인한다.
bool FArduinoAxisTest::RunTest(const FString& Parameters)
{
    float Axis = 9;
    const auto Normalize = [&](int32 Raw, int32 Min, int32 Center, int32 Max, float DeadZone, bool Invert)
    { return ArduinoJoystick::NormalizeAxis(Raw, Min, Center, Max, DeadZone, Invert, Axis); };
    TestTrue(TEXT("center valid"), Normalize(512, 0, 512, 1023, .1f, false));
    TestEqual(TEXT("center neutral"), Axis, 0.f);
    Normalize(0, 0, 512, 1023, .1f, false); TestEqual(TEXT("minimum"), Axis, -1.f);
    Normalize(1023, 0, 512, 1023, .1f, false); TestEqual(TEXT("maximum"), Axis, 1.f);
    Normalize(1023, 0, 512, 1023, .1f, true); TestEqual(TEXT("inverted"), Axis, -1.f);
    Normalize(540, 0, 512, 1023, .1f, false); TestEqual(TEXT("deadzone stops drift"), Axis, 0.f);
    Normalize(300, 100, 300, 900, .1f, false); TestEqual(TEXT("asymmetric center"), Axis, 0.f);
    Normalize(630, 100, 300, 900, .1f, false); TestTrue(TEXT("continuous outside deadzone"), FMath::IsNearlyEqual(Axis, .5f, .00001f));
    TestFalse(TEXT("invalid calibration rejected"), Normalize(512, 512, 512, 1023, .1f, false));
    TestEqual(TEXT("invalid calibration neutral"), Axis, 0.f);
    TestFalse(TEXT("invalid deadzone"), Normalize(512, 0, 512, 1023, 1.f, false));
    TestFalse(TEXT("fresh input"), ArduinoJoystick::IsStale(10.0, 10.49, .5f));
    TestTrue(TEXT("timeout boundary"), ArduinoJoystick::IsStale(10.0, 10.5, .5f));
    // [학습] 같은 5초 동안 30/60/120 FPS로 적분해 약 1500 units가 되는지 비교한다.
    for (int32 FPS : {30, 60, 120})
    {
        FVector Distance = FVector::ZeroVector;
        for (int32 Frame = 0; Frame < FPS * 5; ++Frame)
        { Distance += ArduinoJoystick::CalculateMovement(FVector2D(1, 0), 300, 1.f / FPS); }
        TestTrue(TEXT("frame independent 1500 units"), FMath::IsNearlyEqual(Distance.X, 1500.0, .1));
        TestEqual(TEXT("Z fixed"), Distance.Z, 0.0);
    }
    TestTrue(TEXT("diagonal speed bounded"), FMath::IsNearlyEqual(ArduinoJoystick::CalculateMovement(FVector2D(1, 1), 300, 1).Size(), 300.0, .01));
    TestTrue(TEXT("small input retained"), FMath::IsNearlyEqual(ArduinoJoystick::CalculateMovement(FVector2D(.5, 0), 300, 1).X, 150.0, .01));
    return true;
}
#endif
