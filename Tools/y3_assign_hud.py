import unreal


def log(message):
    unreal.log("[Y3 Assign HUD] " + message)


def main():
    hud_class = unreal.load_object(None, "/Game/UI/HUD/BP_Y3HUD.BP_Y3HUD_C")
    if not hud_class:
        raise RuntimeError("BP_Y3HUD generated class not found")

    game_modes = [
        "/Game/Core/GameModes/BP_BattleGameMode",
        "/Game/Core/GameModes/BP_MainMenuGameMode",
        "/Game/Core/GameModes/BP_SelectionGameMode",
    ]

    updated = []
    for gm_path in game_modes:
        gm = unreal.EditorAssetLibrary.load_asset(gm_path)
        gm_class = unreal.load_object(None, f"{gm_path}.{gm_path.rsplit('/', 1)[1]}_C")
        if not gm or not gm_class:
            raise RuntimeError(f"failed to load game mode: {gm_path}")
        cdo = unreal.get_default_object(gm_class)
        cdo.set_editor_property("HUDClass", hud_class)
        unreal.EditorAssetLibrary.save_loaded_asset(gm)
        updated.append(gm_path)
        log(f"{gm_path} HUDClass -> /Game/UI/HUD/BP_Y3HUD")

    return {"updated": updated}


RESULT = main()
print(RESULT)
