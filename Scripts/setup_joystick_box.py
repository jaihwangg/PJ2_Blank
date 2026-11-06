# [학습] 언리얼 에디터 Python 도구다. 일반 Python이나 Uno에서 실행하는 파일이 아니며 매 프레임 게임 로직도 아니다.
"""Run with UnrealEditor-Cmd -EnablePlugins=PythonScriptPlugin -run=pythonscript -script=<this file>."""
import json
import os
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assets = unreal.EditorAssetLibrary
bp_path = "/Game/Blueprints/Arduino/BP_JoystickBox"
map_path = "/Game/Maps/L_JoystickBoxTest"

# Preserve the original test map and record its contents without changing it.
# [학습] 원본 테스트 맵의 구성을 기록하고 원본 에셋은 보존한다.
inventory = []
if assets.does_asset_exist("/Game/Maps/L_ArduinoTest"):
    levels.load_level("/Game/Maps/L_ArduinoTest")
    inventory = [{"label": a.get_actor_label(), "class": a.get_class().get_name()}
                 for a in actors.get_all_level_actors()]

if not assets.does_asset_exist(bp_path):
    # [학습] 이동 로직이 구현된 C++ Actor를 부모로 하는 Blueprint를 만든다.
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ArduinoJoystickBox)
    bp = asset_tools.create_asset("BP_JoystickBox", "/Game/Blueprints/Arduino", unreal.Blueprint, factory)
    assert bp, "Blueprint creation failed"
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert assets.save_loaded_asset(bp), "Blueprint save failed"

if not assets.does_asset_exist(map_path):
    assert levels.new_level(map_path), "Test map creation failed"
else:
    levels.load_level(map_path)

# [학습] 이미 상자가 배치돼 있으면 다시 배치하지 않는다. 현재 맵을 매번 초기화하는 용도로 쓰지 않는다.
if not any(isinstance(a, unreal.ArduinoJoystickBox) for a in actors.get_all_level_actors()):
    box_class = assets.load_blueprint_class(bp_path)
    box = actors.spawn_actor_from_class(box_class, unreal.Vector(0, 0, 50))
    box.set_actor_label("JoystickBox")
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -10))
    floor.set_actor_label("JoystickTestFloor")
    mesh = floor.static_mesh_component
    mesh.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(50, 50, .2))
    light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 600), unreal.Rotator(-55, -30, 0))
    light.set_actor_label("JoystickTestSun")
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(-1000, -1000, 1300))
    # [학습] 카메라 위치에서 상자를 바라보는 회전을 계산한다. 이후 에디터에서 시점을 직접 바꿀 수 있다.
    rotation = unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(), unreal.Vector(0, 0, 50))
    camera.set_actor_rotation(rotation, False)
    camera.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0)
    camera.set_actor_label("JoystickTestCamera")
    assert levels.save_current_level(), "Test map save failed"

boxes = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.ArduinoJoystickBox)]
assert len(boxes) == 1, "Expected one joystick box"
box = boxes[0]
assert box.serial.baud_rate == 115200
assert box.move_speed == 300
# [학습] 생성 결과와 기본 설정을 파일로 남겨 에셋 생성 성공을 확인한다.
report = {"original_map_actors": inventory, "blueprint": bp_path, "map": map_path,
          "box_class": box.get_class().get_name(), "port": box.serial.port_name,
          "baud": box.serial.baud_rate, "status": "assets_created_and_checked"}
report_path = os.path.join(unreal.Paths.project_saved_dir(), "JoystickAssetReport.json")
with open(report_path, "w", encoding="utf-8") as f:
    json.dump(report, f, ensure_ascii=False, indent=2)
unreal.log("JOYSTICK_ASSETS_OK " + json.dumps(report))
