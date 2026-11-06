// [학습] Uno에서 실행하는 스케치: A0/A1을 읽고 JOY,X,Y 줄을 PC로 보낸다. 버튼과 역방향 출력은 아직 사용하지 않는다.
// Uno: GND -> GND, +5V -> 5V, VRx -> A0, VRy -> A1. SW unused.
// [학습] 20ms 간격은 목표 약 50회/초다. 이 값이 전체 입력 지연을 정확히 20ms로 고정하지는 않는다.
const unsigned long SampleIntervalMs = 20;
// [학습] 마지막으로 보낸 시각을 기억한다. unsigned 정수 시간의 뺄셈은 millis 값의 순환도 처리할 수 있다.
unsigned long LastSampleMs = 0;

// [학습] 전원 켜기 또는 보드 재시작 때 한 번 실행한다. 모니터와 UE도 동일한 115200 속도로 맞춘다.
void setup()
{
    Serial.begin(115200);
}

// [학습] 계속 반복되지만, 전송 간격이 지나지 않으면 return으로 이번 호출만 끝낸다.
void loop()
{
    const unsigned long now = millis();
    if (now - LastSampleMs < SampleIntervalMs) { return; }
    // [학습] 현재 시각을 기준으로 다음 주기를 잡는다. 늦어진 옛 샘플을 한꺼번에 몰아서 보내지 않는다.
    LastSampleMs = now; // Do not burst old samples after a delay; unsigned subtraction handles rollover.
    // [학습] 기본 Uno ADC는 입력 전압을 0~1023의 정수로 읽는다. 가운데가 정확히 512인지는 보정으로 맞춘다.
    const int x = analogRead(A0);
    const int y = analogRead(A1);
    // [학습] 숫자를 문자로 출력한다. 쉼표는 항목 경계이고 마지막 println이 CRLF 줄 끝을 붙인다.
    Serial.print("JOY,");
    Serial.print(x);
    Serial.print(',');
    Serial.println(y);
}
