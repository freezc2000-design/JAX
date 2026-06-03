import unreal


def log(message):
    unreal.log("[Y3 HUD Branch] " + message)


def ensure_duplicate(src, dst):
    if unreal.EditorAssetLibrary.does_asset_exist(dst):
        log(f"exists: {dst}")
        return unreal.EditorAssetLibrary.load_asset(dst)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    dst_dir, dst_name = dst.rsplit("/", 1)
    asset = asset_tools.duplicate_asset(dst_name, dst_dir, unreal.EditorAssetLibrary.load_asset(src))
    if not asset:
        raise RuntimeError(f"failed to duplicate {src} -> {dst}")
    log(f"duplicated: {src} -> {dst}")
    return asset


def remove_widgets(widget_asset_path, widget_names):
    removed = []
    for name in widget_names:
        widget = unreal.find_object(None, f"{widget_asset_path}.{widget_asset_path.rsplit('/', 1)[1]}:WidgetTree.{name}")
        if not widget:
            log(f"missing widget, skip: {name}")
            continue
        parent = widget.get_parent()
        if parent:
            parent.remove_child(widget)
            removed.append(name)
            log(f"removed widget: {name}")
        else:
            log(f"widget has no parent, skip: {name}")
    return removed


def main():
    overlay_src = "/Game/Blueprints/UI/OverLay/WBP_Overlay"
    overlay_dst = "/Game/UI/HUD/WBP_Y3BattleOverlay"
    hud_src = "/Game/Blueprints/UI/HUD/BP_AuraHUD"
    hud_dst = "/Game/UI/HUD/BP_Y3HUD"

    overlay = ensure_duplicate(overlay_src, overlay_dst)
    hud = ensure_duplicate(hud_src, hud_dst)

    removed = remove_widgets(overlay_dst, ["Btn_AttributeMenu", "Btn_SpellMenu"])

    hud_class_path = "/Game/UI/HUD/BP_Y3HUD.BP_Y3HUD_C"
    hud_class = unreal.load_object(None, hud_class_path)
    if not hud_class:
        raise RuntimeError("failed to resolve BP_Y3HUD generated class")

    hud_cdo = unreal.get_default_object(hud_class)
    overlay_class_path = "/Game/UI/HUD/WBP_Y3BattleOverlay.WBP_Y3BattleOverlay_C"
    overlay_class = unreal.load_object(None, overlay_class_path)
    if not overlay_class:
        raise RuntimeError("failed to resolve WBP_Y3BattleOverlay generated class")

    hud_cdo.set_editor_property("OverlayWidgetClass", overlay_class)
    log(f"BP_Y3HUD OverlayWidgetClass -> {overlay_class_path}")

    unreal.EditorAssetLibrary.save_loaded_asset(overlay)
    unreal.EditorAssetLibrary.save_loaded_asset(hud)

    return {
        "overlay": overlay_dst,
        "hud": hud_dst,
        "removed_widgets": removed,
    }


RESULT = main()
print(RESULT)
