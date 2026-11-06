# Joystick Box Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task. User authorized implementation from PRD/SDD; proceed in this session.

**Goal:** Uno 조이스틱으로 UE 5.8 상자를 이동한다.
**Architecture:** 기존 Windows 시리얼 컴포넌트 → 조이스틱 보정 컴포넌트 → 재사용 가능한 C++ 상자 베이스와 Blueprint.
**Tech Stack:** UE 5.8 C++, Windows COM, Arduino C++, UE Automation, editor Python for assets only.
**Spec:** PRD.md, SDD.md (같은 폴더).

## Global Constraints
- 115200 baud / 8N1, 20ms 주기, JOY,X,Y LF 메시지, ADC 0~1023.
- 64바이트 줄 제한, 읽기 256바이트 / Tick 1024바이트.
- 축별 데드존 0.10, 입력 만료 0.5초, 속도 300 units/s.
- 월드 X/Y, Z 고정, 대각선 속도 제한. 출력 명령·버튼·자동 재연결 제외.

## Review Focus
- 긴 줄 폐기 중 정상 줄 시작을 잘못 해석하지 않는다.
- 잘못된 메시지가 입력 만료 시간을 연장하지 않는다.
- 잘못된 보정값·NaN 설정이 이동에 전파되지 않는다.
- 포트 재시도·종료에서 오래된 버퍼와 입력을 재사용하지 않는다.
- 게임 재개 시 오래된 입력으로 이동하지 않는다.

## Task 1 — Protocol and calibration
- [x] Create Tests/ArduinoJoystickTests.cpp: strict parser, split/multiple lines, overflow recovery, normalization/inversion/config validation, timeout, movement distance.
- [x] Build to observe missing interfaces, then implement ArduinoJoystickProtocol.h/.cpp.
- [ ] Interfaces: FArduinoLineBuffer::Push(uint8,FString&), ArduinoJoystick::ParseLine(const FString&,FIntPoint&), NormalizeAxis(int32,int32,int32,int32,float,bool,float&), IsStale(double,double,float), CalculateMovement(FVector2D,float,float).
- [x] Run PJ2.Arduino automation tests and confirm pass.

## Task 2 — Serial and joystick components
- [x] Extend ArduinoSerialComponent.h/.cpp: bounded immediate reads, line/state events, LastError, idempotent close, guarded Windows code; preserve existing Blueprint methods.
- [x] Create ArduinoJoystickComponent.h/.cpp: bind serial events, ParseLine → normalization → Axis, states and monotonic timeout, tick prerequisites and reset.
- [x] Add component integration automation: bad line does not replace input, close/reset neutralizes, invalid calibration neutralizes, missing port reports error.
- [x] Build and run whole project automation group.

## Task 3 — Firmware and box assets
- [x] Add Arduino/JoystickBox/JoystickBox.ino with millis-based 20ms analogRead+Serial output.
- [x] Create ArduinoJoystickBox.h/.cpp actor: cube root, serial+joystick components, tick ordering, CalculateMovement, diagnostics and Blueprint-accessible retry.
- [x] Add editor-only setup script: inspect existing map, create BP_JoystickBox, create safe test map without deleting original assets.
- [x] Compile, generate assets through Unreal editor Python, run editor automation and render test map. Windows game target build succeeded; final packaging deferred by user on 2026-11-06.
- [x] Write JOYSTICK_SETUP.md with wiring, upload, COM3 confirmation, map/PIE use and hardware acceptance checklist.

## Execution ledger
- Environment: project is not a Git repository; no worktree or commits possible. Original edited files backed up under chat work/backup.
- Ruling: keep BP behavior in a reusable native actor parent; BP remains editable and avoids generating fragile graph nodes. Cost: movement graph is read in C++.
- Ruling: use a separate test map if existing map content cannot safely be reused; preserve original map. Cost: user opens the generated test map explicitly.

### Implementation ledger
- Tasks 1–2: editor build passed; 4 project automation tests passed. Parser lowercase rejection bug caught and fixed; float assertion uses 1e-5 tolerance.
- Task 3: Blueprint and separate map created and validated by editor Python. Firmware compiled and linked with installed AVR toolchain: 2772 program bytes, 217 data bytes; HEX generated.
- Ruling: Arduino CLI cannot traverse hardware packages in this sandbox. Use installed compiler/core directly via Scripts/build_uno.ps1; no firmware upload performed. Cost: IDE upload remains a user step.
- Ruling: implement existing SendText API to resolve pre-existing LNK2019. No output command protocol added. Cost: direct SendText uses a bounded synchronous write; phase-one movement never calls it.
- Final review (fresh subagent): one Important reconnect-during-delivery defect. Regression failed with two old messages instead of one, and old prefix contaminating fresh data. Fixed with line-buffer generation invalidation; final full suite 4/4 passed, zero failures.
- Final: minor (deferred): Receiving delegate broadcasts before public Axis updates. Box movement uses GetCurrentAxis after update and is unaffected. Blueprint listeners should call GetCurrentAxis in that callback.
- Physical checks pending: COM3 opened but no data arrived within timeout; board upload and hands-on movement were not verified.
- Packaging: game target compiled; first cook failed on engine log directory permissions. Redirected AutomationTool saved logs to project Saved/AutomationTool and retried.
- Scope update 2026-11-06: user chose editor interaction first and packaging later. Stopped the packaging process; editor deliverable is ready for hardware upload and hands-on checks.
