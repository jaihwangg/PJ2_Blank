// [학습] 장치 없이도 시험할 수 있는 줄 조립·형식 검사·축 보정·이동 계산의 선언이다.
#pragma once
#include "CoreMinimal.h"

// Byte framing is shared by the real serial reader and automation tests.
// [학습] 시리얼 읽기 한 번에 줄이 완성된다고 가정하지 않고 바이트를 차곡차곡 모은다.
class FArduinoLineBuffer
{
public:
    // [학습] 바이트 하나를 넣는다. 완성된 줄이 있을 때만 true이며 OutLine에 줄 끝을 제거한 문자열을 쓴다.
    bool Push(uint8 Byte, FString& OutLine);
    // [학습] 한 번 읽은 바이트 묶음에서 완성된 줄마다 콜백을 부른다. 콜백 중 Reset되면 옛 묶음을 중단한다.
    void Consume(const uint8* Bytes, int32 Count, TFunctionRef<void(const FString&)> OnLine);
    // [학습] 연결 닫기·재시도 시 부분 줄과 폐기 상태를 지운다. 이미 읽은 옛 배치도 무효화한다.
    void Reset();
private:
    // [학습] 아직 LF를 못 만난 부분 문자열이다. 정상 입력에서도 여러 번의 읽기에 걸쳐 쌓일 수 있다.
    FString Pending;
    // [학습] 너무 길거나 잘못된 바이트가 있는 줄은 다음 LF까지 통째로 버린다.
    bool bDiscarding = false;
    // [학습] Reset 횟수를 세는 세대 번호. 핸들 값이 재사용되어도 연결 초기화를 구분한다.
    uint64 Generation = 0;
};

namespace ArduinoJoystick
{
    // [학습] 성공한 경우에만 OutRaw에 정수 두 개를 쓴다. JOY의 대소문자와 0~1023 범위를 엄격하게 검사한다.
    bool ParseLine(const FString& Line, FIntPoint& OutRaw);
    // [학습] 원본 한 축을 -1~1로 바꾸고 데드존·반전을 적용한다. 보정 설정 오류는 false와 0을 돌려준다.
    bool NormalizeAxis(int32 Raw, int32 Min, int32 Center, int32 Max, float DeadZone, bool bInvert, float& OutAxis);
    // [학습] 게임 시간 배율과 별개인 경과 시각으로 유효 입력이 너무 오래되었는지 판단한다.
    bool IsStale(double LastValid, double Now, float Timeout);
    // [학습] 대각선 길이를 제한한 입력에 초당 속도와 프레임 시간을 곱해 월드 X/Y 이동량을 만든다.
    FVector CalculateMovement(FVector2D Axis, float Speed, float DeltaSeconds);
}
