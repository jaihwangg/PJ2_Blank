// [학습] 읽은 바이트 → 완성된 줄 → 검증된 숫자 → 보정 축 → 이동량의 순수 계산을 구현한다.
#include "ArduinoJoystickProtocol.h"

// [학습] 이벤트 수신자가 동기적으로 포트를 닫고 다시 열 수도 있다. 시작 세대와 달라지면 옛 배치를 더 처리하지 않는다.
void FArduinoLineBuffer::Consume(const uint8* Bytes, int32 Count, TFunctionRef<void(const FString&)> OnLine)
{
    const uint64 BatchGeneration = Generation;
    for (int32 Index = 0; Index < Count && Generation == BatchGeneration; ++Index)
    {
        FString Line;
        if (Push(Bytes[Index], Line)) { OnLine(Line); }
    }
}

// [학습] LF가 문장 끝이다. CRLF에서 CR만 제거하며, LF 전까지는 바이트를 저장한다.
bool FArduinoLineBuffer::Push(uint8 Byte, FString& OutLine)
{
    if (Byte == '\n')
    {
        const bool bReady = !bDiscarding && !Pending.IsEmpty();
        if (bReady)
        {
            Pending.RemoveFromEnd(TEXT("\r"));
            OutLine = Pending;
        }
        Pending.Reset();
        bDiscarding = false;
        return bReady && !OutLine.IsEmpty();
    }
    if (!bDiscarding)
    {
        // [학습] 무한한 부분 줄 누적을 막는다. NUL과 ASCII 밖 바이트도 거부하며 다음 LF에서 정상 조립으로 돌아간다.
        if (Pending.Len() == 64 || Byte > 127 || Byte == 0)
        {
            Pending.Reset();
            bDiscarding = true;
        }
        else { Pending.AppendChar(static_cast<TCHAR>(Byte)); }
    }
    return false;
}

// [학습] 일반적인 줄 완성과 연결 초기화는 다르다. 연결 초기화에서만 세대를 올려 이전 배치를 무효화한다.
void FArduinoLineBuffer::Reset()
{
    ++Generation; // A synchronous listener may close/reopen while a read batch is being delivered.
    Pending.Reset();
    bDiscarding = false;
}

// [학습] 쉼표 세 항목과 JOY 접두사, 부호 없는 숫자만 허용한다. 잘못된 값을 0으로 억지 변환하지 않는다.
bool ArduinoJoystick::ParseLine(const FString& Line, FIntPoint& OutRaw)
{
    TArray<FString> Tokens;
    Line.ParseIntoArray(Tokens, TEXT(","), false);
    if (Tokens.Num() != 3 || !Tokens[0].Equals(TEXT("JOY"), ESearchCase::CaseSensitive)) { return false; }
    FIntPoint Value;
    for (int32 Axis = 0; Axis < 2; ++Axis)
    {
        const FString& Token = Tokens[Axis + 1];
        if (Token.IsEmpty() || Token.Len() > 4) { return false; }
        int32 Number = 0;
        for (TCHAR C : Token)
        {
            if (C < TEXT('0') || C > TEXT('9')) { return false; }
            // [학습] 예를 들어 5를 읽은 뒤 1을 만나면 5*10+1=51이다. 자리별로 정수를 만든다.
            Number = Number * 10 + C - TEXT('0');
        }
        if (Number > 1023) { return false; }
        (Axis == 0 ? Value.X : Value.Y) = Number;
    }
    // [학습] 모든 검사를 통과한 뒤에만 호출한 쪽의 결과를 바꾼다.
    OutRaw = Value;
    return true;
}

// [학습] 항상 출력부터 0으로 만든다. 분모가 0이 되거나 NaN 설정이 이동에 섞이지 않도록 검사한다.
bool ArduinoJoystick::NormalizeAxis(int32 Raw, int32 Min, int32 Center, int32 Max, float DeadZone, bool bInvert, float& OutAxis)
{
    OutAxis = 0;
    if (Min < 0 || Max > 1023 || Min >= Center || Center >= Max || !FMath::IsFinite(DeadZone) || DeadZone < 0 || DeadZone >= 1)
    { return false; }
    // [학습] 중심 양쪽의 실제 거리가 다를 수 있어 분모를 따로 쓴다. 최종 값은 -1~1로 제한한다.
    const float N = FMath::Clamp(static_cast<float>(Raw - Center) / (Raw >= Center ? Max - Center : Center - Min), -1.f, 1.f);
    // [학습] 데드존 안은 그대로 0이다. 밖은 남은 구간을 다시 0~1로 펼친 뒤 필요하면 부호를 뒤집는다.
    if (FMath::Abs(N) > DeadZone)
    { OutAxis = FMath::Sign(N) * (FMath::Abs(N) - DeadZone) / (1 - DeadZone) * (bInvert ? -1 : 1); }
    return true;
}

// [학습] 유효 입력이 Timeout 이상 없으면 만료다. 시간 설정 자체가 잘못된 경우도 정지 쪽으로 판단한다.
bool ArduinoJoystick::IsStale(double LastValid, double Now, float Timeout)
{
    return !FMath::IsFinite(Timeout) || Timeout <= 0 || Now - LastValid >= Timeout;
}

// [학습] 잘못된 소수와 음수 속도·시간은 이동에 사용하지 않는다. Z 이동량은 항상 0이다.
FVector ArduinoJoystick::CalculateMovement(FVector2D Axis, float Speed, float DeltaSeconds)
{
    if (Axis.ContainsNaN() || !FMath::IsFinite(Speed) || !FMath::IsFinite(DeltaSeconds) || Speed < 0 || DeltaSeconds < 0)
    { return FVector::ZeroVector; }
    // [학습] (1,1)의 길이는 약 1.414이므로 길이 1로 줄인다. 작은 입력은 정규화하지 않아 기울기에 비례한 속도를 유지한다.
    if (Axis.SizeSquared() > 1) { Axis.Normalize(); }
    // [학습] 속도 단위는 units/초, DeltaSeconds는 초이므로 곱한 결과는 이번 프레임의 이동 거리다.
    return FVector(Axis.X, Axis.Y, 0) * Speed * DeltaSeconds;
}
