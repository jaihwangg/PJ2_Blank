# 조이스틱 상자 이동 실행 안내

대상 프로젝트: `C:\Work\Unreal5.8\PJ2_Blank`

현재 범위는 에디터 Play에서 조작 확인입니다. Windows 패키징은 사용자 선택으로 추후 진행합니다.

## 1. 배선

Uno의 USB 전원을 분리하고 연결합니다.

| 조이스틱 | Uno |
|---|---|
| GND | GND |
| +5V | 5V |
| VRx | A0 |
| VRy | A1 |
| SW | 미연결 |

## 2. Uno 업로드

1. Arduino IDE에서 프로젝트의 `Arduino/JoystickBox/JoystickBox.ino`를 엽니다.
2. 보드를 Arduino Uno로 선택하고 실제 포트를 지정합니다. 작업 환경에서 COM3가 열리는 것을 확인했으나 JOY 데이터는 아직 수신되지 않았습니다.
3. 업로드 후 시리얼 모니터를 115200으로 열어 `JOY,512,508` 형태를 확인합니다.
4. **시리얼 모니터를 닫고** 언리얼을 실행합니다. 포트를 동시에 사용할 수 없습니다.

스케치는 Uno 대상 컴파일·링크와 HEX 생성을 확인했습니다. 보드의 기존 프로그램을 덮어쓰는 업로드는 수행하지 않았습니다.

## 3. 언리얼 실행

1. `PJ2_Blank.uproject`를 UE 5.8로 엽니다.
2. 콘텐츠 브라우저에서 `/Game/Maps/L_JoystickBoxTest`를 엽니다.
3. 레벨의 `JoystickBox`를 선택합니다.
4. Details에서 `Serial` 컴포넌트의 Port Name을 실제 COM 번호로, Baud Rate를 115200으로 설정합니다.
5. Play를 누릅니다. 상자는 자동으로 포트를 엽니다.
6. 화면의 입력 상태가 Receiving으로 바뀌면 조이스틱으로 이동합니다. 통신이 끊긴 경우 게임 화면에 포커스를 둔 뒤 **R**로 수동 재연결합니다.

원본 `L_ArduinoTest`와 기본 시작 맵 설정은 보존했습니다. 새 맵에는 `BP_JoystickBox`, 바닥, 조명, 고정 카메라가 있습니다. 이동 로직은 Blueprint의 부모 C++ 클래스 `AArduinoJoystickBox`에 있습니다.

## 4. 보정

- `Joystick` 컴포넌트에서 각 축의 Min / Center / Max를 조절합니다. 기본값은 0 / 512 / 1023입니다.
- 손을 놓은 원시값을 Center로 설정합니다. 실제 양 끝 값을 Min / Max에 반영합니다.
- 반대 방향이면 Invert X / Invert Y를 켭니다. VRx는 월드 X, VRy는 월드 Y입니다.
- Dead Zone은 기본 0.10입니다. 중립에서 떨리면 조금 늘립니다.
- 상자의 Move Speed는 기본 300 units/s입니다.
- 모든 보정은 `0 <= Min < Center < Max <= 1023`이어야 합니다. 잘못된 설정은 이동을 0으로 만듭니다.

## 5. 문제 확인

| 상태 | 확인 |
|---|---|
| Error / Cannot open | 포트 번호, 케이블, Arduino IDE 시리얼 모니터 점유 확인 |
| WaitingForInput | 스케치 업로드 여부, 양쪽 115200, Uno 재시작 대기 확인 |
| Stale | 정상 입력 수신 후 0.5초 이상 끊김. 연결·전송 주기 확인 |
| 입력값은 있는데 정지 | 데드존, 보정 오류 메시지, 속도, 연결 상태 확인 |
| 화면에서 방향이 예상과 다름 | 월드 축 기준 이동이므로 카메라 시점과 축 반전 설정 확인 |

## 6. 실제 장치 인수 체크

- [ ] 올바른 포트에서 Receiving 확인
- [ ] 중립 상태 10초 동안 정지
- [ ] 네 방향·대각선 이동과 고정 Z 확인
- [ ] 대각선 최대 속도와 축 반전 확인
- [ ] 케이블 분리 시 입력 만료 후 정지, 재연결 후 R로 복구
- [ ] PIE 시작·종료 5회 후 포트 재개방
- [ ] Windows 패키지에서 동일 동작

이 체크는 사용자가 실제 조이스틱을 움직여 확인해야 합니다. 자동화 테스트 통과가 물리 장치 검증을 대신하지 않습니다.

## 7. 개발 도구

- `Scripts/setup_joystick_box.py`: UE editor Python으로 Blueprint와 테스트 맵 생성. 기존 원본 맵은 변경하지 않습니다.
- `Scripts/build_uno.ps1`: 설치된 Arduino AVR core 1.8.8과 avr-gcc로 오프라인 펌웨어 빌드. 다른 설치 버전은 인수로 지정합니다.
- `Scripts/package_joystick_box.ps1`: Windows Development 패키징. 셰이더 작업자 실행이 제한된 환경은 `-NoShaderWorker`를 지정합니다.
- 패키지 실행 시 `PJ2_Blank.exe /Game/Maps/L_JoystickBoxTest`처럼 새 맵을 지정합니다. 프로젝트 기본 시작 맵은 원본을 보존했습니다.
- UE Automation 필터: `PJ2.Arduino`
- 자동화 보고서: 프로젝트의 `Saved/JoystickTests/index.json`
- 에셋 보고서: 프로젝트의 `Saved/JoystickAssetReport.json`

상세 요구사항은 PRD.md, 설계는 SDD.md를 참고합니다.
