# 구현 결과 — Arduino 조이스틱 상자 이동

작성: 2026-11-06. 대상: `C:\Work\Unreal5.8\PJ2_Blank`.

## 적용한 기능

- 기존 UArduinoSerialComponent를 확장해 115200 / 8N1 수신, 줄 단위 조립, Tick당 작업량 제한, 오류·종료 처리를 구현했다.
- 조이스틱 파싱·보정 컴포넌트를 추가했다. 축별 Min/Center/Max, 데드존, 반전, 0.5초 입력 만료를 지원한다.
- 상자 부모 C++ 액터와 BP_JoystickBox를 추가했다. X/Y 이동, Z 유지, DeltaSeconds 적용, 대각선 속도 제한, 화면 진단, R 키 수동 재연결을 제공한다.
- 원본 맵을 보존하고 `/Game/Maps/L_JoystickBoxTest`에 상자·바닥·조명·고정 카메라를 구성했다.
- Arduino/JoystickBox/JoystickBox.ino에 A0/A1 입력을 20ms마다 JOY,X,Y 메시지로 보내는 펌웨어를 작성했다.

## 검증 결과

| 검증 | 결과 |
|---|---|
| UE 5.8.4 Editor Development 빌드 | 성공 |
| Windows Game Development 실행 파일 빌드 | 성공 |
| PJ2.Arduino.BoxMovement | 성공 — 실제 액터 이동 및 초기화 후 정지 |
| PJ2.Arduino.CalibrationAndMovement | 성공 — 축 보정·만료 경계·FPS별 이동·대각선 제한 |
| PJ2.Arduino.ComponentInput | 성공 — 무효 입력·만료·복귀·설정 오류·없는 포트 |
| PJ2.Arduino.Protocol | 성공 — 엄격한 형식·분할·다중 줄·길이 초과 복구·재연결 도중 이전 배치 폐기 |
| UE editor Python 에셋 생성·검사 | 성공 — Blueprint 클래스, 상자 1개, COM3 / 115200 / 속도 300 확인 |
| Uno 펌웨어 컴파일·링크·HEX 생성 | 성공 — 프로그램 2772바이트, 데이터 217바이트 |
| COM3 포트 확인 | 열기 성공, 데이터 수신은 타임아웃 |
| 실제 테스트 맵 렌더링 | 성공 — 1280×720 캡처, native 시리얼 Open → WaitingForInput → Closed 로그 확인 |
| 물리 조이스틱 조작·PIE 5회 반복 | 미검증 — 업로드와 사용자의 실제 조작 필요 |
| Windows Cook / Stage / Archive | 사용자 선택으로 중단·보류. 에디터 조작 확인 후 추후 진행 |

자동화는 4개 테스트 모두 성공, 실패 0개다. ComponentInput에서 존재하지 않는 포트를 의도적으로 열 때 발생한 진단 경고 1개가 포함된다. 자동화 결과 원문은 [AutomationResults.json](AutomationResults.json), 에셋 결과는 [JoystickAssetReport.json](JoystickAssetReport.json)이다.

테스트 맵 화면: [JoystickBox_Preview.png](JoystickBox_Preview.png).

## 리뷰와 수정

별도 리뷰에서 수신 이벤트 중 Close/Open을 호출하면 이전 배치의 바이트가 새 연결로 넘어올 수 있는 문제를 발견했다. 테스트에서 이전 메시지가 2개 전달되고 새 메시지에 old 접두사가 섞이는 것을 재현했다. 버퍼 Reset 세대 번호를 확인해 기존 배치 처리를 중단하도록 수정한 뒤 전체 테스트를 통과했다.

경미한 후속 항목: Receiving 이벤트 콜백에서 공개 Axis 속성을 바로 읽으면 이전 값일 수 있다. 현재 상자 이동에는 영향이 없으며, 외부 Blueprint 콜백은 GetCurrentAxis()를 호출한다. 상태 이벤트 전에 Axis를 갱신하는 정리는 후속 작업으로 남겼다.

## 설계에서 조정한 사항

1. Blueprint 그래프 대신 재사용 가능한 C++ 부모에서 이동을 구현했다. Blueprint에서 컴포넌트·설정은 편집할 수 있고, 이동 로직은 C++에서 확인한다.
2. 기존 L_ArduinoTest를 수정하지 않고 별도 맵을 생성했다. 사용자는 새 맵을 직접 열어 실행한다. 기본 시작 맵 설정은 그대로다.
3. 기존 SendText 미구현 때문에 발생한 LNK2019를 해결하도록 메서드를 구현했다. 이번 입력 기능은 이를 호출하지 않으며 출력 명령 규약은 추가하지 않았다. 직접 호출 시 최대 100ms 쓰기 타임아웃이 적용된다.
4. Arduino CLI가 샌드박스에서 설치 패키지 경로를 탐색하지 못해, 설치된 AVR core와 compiler를 Scripts/build_uno.ps1로 직접 실행했다. IDE 업로드 절차는 별도로 진행해야 한다.
5. Git 저장소가 아니므로 브랜치·커밋을 만들지 않았다. 수정 전 serial 원본과 uproject는 이 채팅 작업 폴더의 work/backup에 백업했다.

## 실행

[JOYSTICK_SETUP.md](JOYSTICK_SETUP.md)를 따른다. Uno 스케치를 업로드하고 시리얼 모니터를 닫은 뒤 UE에서 L_JoystickBoxTest를 열어 Play를 누른다. 포트는 실제 장치에 맞춘다.

보드에 프로그램을 업로드하거나 실제 하드웨어 완료 판정을 내리지는 않았다. 상자 이동 검증 후 UE→Arduino 출력·버튼 기능을 별도 단계로 확장한다.

사용자가 2026-11-06 에디터 조작을 먼저 확인하고 패키징은 추후 진행하도록 범위를 조정했다. 실행 중인 패키징은 중단했다. 패키징 재개는 Scripts/package_joystick_box.ps1을 사용한다.
