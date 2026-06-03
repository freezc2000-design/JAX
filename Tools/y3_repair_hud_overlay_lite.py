import unreal


def log(message):
    unreal.log("[Y3 HUD Lite] " + message)


def duplicate_if_missing(src, dst):
    if unreal.EditorAssetLibrary.does_asset_exist(dst):
        log(f"exists: {dst}")
        return unreal.EditorAssetLibrary.load_asset(dst)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    dst_dir, dst_name = dst.rsplit("/", 1)
    asset = tools.duplicate_asset(dst_name, dst_dir, unreal.EditorAssetLibrary.load_asset(src))
    if not asset:
        raise RuntimeError(f"failed duplicate: {src} -> {dst}")
    log(f"duplicated: {src} -> {dst}")
    return asset


def set_widget_collapsed(widget_asset_path, widget_name):
    obj_name = widget_asset_path.rsplit("/", 1)[1]
    widget = unreal.find_object(None, f"{widget_asset_path}.{obj_name}:WidgetTree.{widget_name}")
    if not widget:
        raise RuntimeError(f"widget not found: {widget_name}")
    widget.set_editor_property("visibility", unreal.SlateVisibility.COLLAPSED)
    log(f"{widget_name} visibility -> Collapsed")


def main():
    src_overlay = "/Game/Blueprints/UI/OverLay/WBP_Overlay"
    lite_overlay = "/Game/UI/HUD/WBP_Y3BattleOverlay_Lite"
    hud_path = "/Game/UI/HUD/BP_Y3HUD"

    overlay = duplicate_if_missing(src_overlay, lite_overlay)
    set_widget_collapsed(lite_overlay, "Btn_AttributeMenu")
    set_widget_collapsed(lite_overlay, "Btn_SpellMenu")

    hud = unreal.EditorAssetLibrary.load_asset(hud_path)
    hud_class = unreal.load_object(None, "/Game/UI/HUD/BP_Y3HUD.BP_Y3HUD_C")
    overlay_class = unreal.load_object(None, "/Game/UI/HUD/WBP_Y3BattleOverlay_Lite.WBP_Y3BattleOverlay_Lite_C")
    if not hud or not hud_class or not overlay_class:
        raise RuntimeError("failed to load HUD or Lite overlay generated class")

    hud_cdo = unreal.get_default_object(hud_class)
    hud_cdo.set_editor_property("OverlayWidgetClass", overlay_class)
    log("BP_Y3HUD OverlayWidgetClass -> /Game/UI/HUD/WBP_Y3BattleOverlay_Lite")

    unreal.EditorAssetLibrary.save_loaded_asset(overlay)
    unreal.EditorAssetLibrary.save_loaded_asset(hud)

    return {"overlay": lite_overlay, "hud": hud_path}


RESULT = main()
print(RESULT)
