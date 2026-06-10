from __future__ import annotations

import argparse
import os
import struct
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


@dataclass(frozen=True)
class LayerSpec:
    name: str
    box: tuple[int, int, int, int]
    alpha: str = "opaque"


LAYERS: list[LayerSpec] = [
    LayerSpec("avatar_frame", (50, 75, 260, 295), "cutout"),
    LayerSpec("avatar_level_badge", (248, 202, 322, 275), "cutout"),
    LayerSpec("player_hp_bar", (409, 95, 841, 145), "opaque"),
    LayerSpec("player_mp_bar", (409, 164, 841, 214), "opaque"),
    LayerSpec("buff_icon_slot_01", (907, 94, 981, 169), "opaque"),
    LayerSpec("buff_icon_slot_02", (999, 94, 1074, 169), "opaque"),
    LayerSpec("buff_icon_slot_03", (1091, 94, 1166, 169), "opaque"),
    LayerSpec("buff_icon_slot_04", (1184, 94, 1259, 169), "opaque"),
    LayerSpec("buff_icon_slot_05", (1276, 94, 1351, 169), "opaque"),
    LayerSpec("buff_timer_text_10s", (1422, 116, 1457, 140), "cutout"),
    LayerSpec("buff_timer_text_30s", (1488, 116, 1523, 140), "cutout"),
    LayerSpec("buff_timer_text_5s", (1559, 116, 1583, 140), "cutout"),
    LayerSpec("buff_timer_text_15s", (1615, 116, 1652, 140), "cutout"),
    LayerSpec("level_badge", (1785, 102, 1868, 184), "cutout"),
    LayerSpec("ability_slot_empty", (130, 365, 265, 494), "cutout"),
    LayerSpec("ability_slot_filled_01", (386, 366, 510, 490), "cutout"),
    LayerSpec("ability_slot_filled_02", (541, 366, 665, 490), "cutout"),
    LayerSpec("ability_slot_filled_03", (696, 366, 820, 490), "cutout"),
    LayerSpec("ability_slot_filled_04", (852, 366, 976, 490), "cutout"),
    LayerSpec("ability_slot_filled_05", (1016, 366, 1141, 490), "cutout"),
    LayerSpec("ability_slot_filled_06", (1181, 366, 1305, 490), "cutout"),
    LayerSpec("skill_slot_empty_01", (81, 571, 243, 713), "opaque"),
    LayerSpec("skill_slot_empty_02", (273, 571, 434, 713), "opaque"),
    LayerSpec("skill_slot_empty_03", (452, 571, 614, 713), "opaque"),
    LayerSpec("skill_slot_empty_04", (629, 571, 789, 713), "opaque"),
    LayerSpec("skill_slot_empty_05", (803, 571, 963, 713), "opaque"),
    LayerSpec("skill_slot_empty_06", (977, 571, 1137, 713), "opaque"),
    LayerSpec("skill_slot_empty_07", (1150, 571, 1309, 713), "opaque"),
    LayerSpec("skill_icon_filled_01", (81, 768, 243, 922), "opaque"),
    LayerSpec("skill_icon_filled_02", (273, 768, 434, 922), "opaque"),
    LayerSpec("skill_icon_filled_03", (452, 768, 614, 922), "opaque"),
    LayerSpec("skill_icon_filled_04", (629, 768, 789, 922), "opaque"),
    LayerSpec("skill_icon_filled_05", (803, 768, 963, 922), "opaque"),
    LayerSpec("skill_icon_filled_06", (977, 768, 1137, 922), "opaque"),
    LayerSpec("skill_icon_filled_07", (1150, 768, 1309, 922), "opaque"),
    LayerSpec("exp_bar_background", (75, 987, 928, 1024), "cutout"),
    LayerSpec("exp_bar_fill", (963, 996, 1607, 1024), "opaque"),
    LayerSpec("exp_bar_tail", (1604, 987, 1907, 1024), "cutout"),
    LayerSpec("exp_bar_text_sample", (804, 1063, 974, 1127), "cutout"),
]

REFERENCE_SIZE = (2048, 1152)


def scale_layers(size: tuple[int, int]) -> list[LayerSpec]:
    scale_x = size[0] / REFERENCE_SIZE[0]
    scale_y = size[1] / REFERENCE_SIZE[1]
    scaled = []
    for spec in LAYERS:
        left, top, right, bottom = spec.box
        box = (
            round(left * scale_x),
            round(top * scale_y),
            round(right * scale_x),
            round(bottom * scale_y),
        )
        scaled.append(LayerSpec(spec.name, box, spec.alpha))
    return scaled


def pascal_name(name: str) -> bytes:
    raw = name.encode("ascii", "replace")[:255]
    data = bytes([len(raw)]) + raw
    while len(data) % 4:
        data += b"\0"
    return data


def be32(value: int) -> bytes:
    return struct.pack(">I", value)


def be16(value: int) -> bytes:
    return struct.pack(">H", value)


def i16(value: int) -> bytes:
    return struct.pack(">h", value)


def make_alpha(crop: Image.Image, mode: str, bg_rgb: np.ndarray) -> Image.Image:
    rgba = np.array(crop.convert("RGBA"), dtype=np.uint8)
    if mode == "opaque":
        rgba[:, :, 3] = 255
        return Image.fromarray(rgba, "RGBA")

    rgb = rgba[:, :, :3].astype(np.int16)
    delta = np.linalg.norm(rgb - bg_rgb.astype(np.int16), axis=2)
    bright = rgb.mean(axis=2)
    alpha = np.where((delta > 13) | (bright > 38), 255, 0).astype(np.uint8)

    # Keep antialiasing soft around extracted silhouettes.
    alpha_img = Image.fromarray(alpha, "L").filter(ImageFilter.GaussianBlur(0.45))
    rgba[:, :, 3] = np.array(alpha_img, dtype=np.uint8)
    return Image.fromarray(rgba, "RGBA")


def flatten_layers(size: tuple[int, int], layers: list[tuple[LayerSpec, Image.Image]]) -> Image.Image:
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    for spec, img in layers:
        canvas.alpha_composite(img, (spec.box[0], spec.box[1]))
    return canvas


def channel_bytes(img: Image.Image, channel_index: int) -> bytes:
    arr = np.array(img, dtype=np.uint8)
    return arr[:, :, channel_index].tobytes()


def write_psd(path: Path, size: tuple[int, int], layers: list[tuple[LayerSpec, Image.Image]]) -> None:
    width, height = size
    records = bytearray()
    channel_data = bytearray()

    for spec, img in layers:
        left, top, right, bottom = spec.box
        layer_width = right - left
        layer_height = bottom - top
        channels = [
            (0, channel_bytes(img, 0)),
            (1, channel_bytes(img, 1)),
            (2, channel_bytes(img, 2)),
            (-1, channel_bytes(img, 3)),
        ]

        records += be32(top) + be32(left) + be32(bottom) + be32(right)
        records += be16(len(channels))
        for channel_id, data in channels:
            records += i16(channel_id) + be32(2 + len(data))
        records += b"8BIM" + b"norm"
        records += bytes([255, 0, 8, 0])

        extra = bytearray()
        extra += be32(0)  # layer mask data
        extra += be32(0)  # blending ranges
        extra += pascal_name(spec.name)
        records += be32(len(extra)) + extra

        expected = layer_width * layer_height
        for _, data in channels:
            if len(data) != expected:
                raise ValueError(f"Bad channel length for {spec.name}")
            channel_data += be16(0) + data

    layer_info = bytearray()
    layer_info += i16(len(layers))
    layer_info += records
    layer_info += channel_data
    if len(layer_info) % 2:
        layer_info += b"\0"

    layer_mask_section = be32(len(layer_info)) + layer_info

    composite = flatten_layers(size, layers)
    composite_channels = b"".join(channel_bytes(composite, idx) for idx in range(4))

    with path.open("wb") as f:
        f.write(b"8BPS")
        f.write(be16(1))
        f.write(b"\0" * 6)
        f.write(be16(4))
        f.write(be32(height))
        f.write(be32(width))
        f.write(be16(8))
        f.write(be16(3))
        f.write(be32(0))  # color mode data
        f.write(be32(0))  # image resources
        f.write(be32(len(layer_mask_section)))
        f.write(layer_mask_section)
        f.write(be16(0))
        f.write(composite_channels)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("--out-dir", type=Path, default=Path("Saved/CodexExports/HUD_UI_Layers"))
    parser.add_argument("--psd-name", default="HUD_UI_Layers.psd")
    args = parser.parse_args()

    src = Image.open(args.source).convert("RGBA")
    width, height = src.size
    bg_rgb = np.array(src.getpixel((10, 10))[:3], dtype=np.uint8)
    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    prepared: list[tuple[LayerSpec, Image.Image]] = []
    manifest_lines = ["name,x,y,width,height,file"]
    for spec in scale_layers(src.size):
        left, top, right, bottom = spec.box
        if left < 0 or top < 0 or right > width or bottom > height:
            raise ValueError(f"{spec.name} box outside source image: {spec.box} vs {src.size}")
        crop = src.crop(spec.box)
        rgba = make_alpha(crop, spec.alpha, bg_rgb)
        png_path = out_dir / f"{spec.name}.png"
        rgba.save(png_path)
        prepared.append((spec, rgba))
        manifest_lines.append(f"{spec.name},{left},{top},{right-left},{bottom-top},{png_path.name}")

    (out_dir / "manifest.csv").write_text("\n".join(manifest_lines) + "\n", encoding="utf-8")
    write_psd(out_dir / args.psd_name, (width, height), prepared)
    flatten_layers((width, height), prepared).save(out_dir / "HUD_UI_Layers_preview.png")
    print(f"Wrote {len(prepared)} PNG layers")
    print(out_dir / args.psd_name)
    print(out_dir / "manifest.csv")


if __name__ == "__main__":
    main()
