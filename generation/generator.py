from PIL import Image
import argparse
import json
from pathlib import Path
from typing import Optional
from itertools import combinations
from dataclasses import dataclass
import numpy as np

# this script is a heaping mess...
# but I really don't want to refactor it because it's a worthless configure script which doesn't really need to be fast and really doesn't need to be expanded/touched often

NAMESPACE = "xaero"
STATE_TYPE = "StateLookup"
STATE_NAME = "defaultStateLookup"
BIOME_TYPE = "BiomeLookup"
BIOME_NAME = "defaultBiomeLookup"

MODELS_CACHE = {}
TEXTURES_CACHE = {}
BLOCKSTATE_CACHE = {}

@dataclass
class Color :
    @staticmethod
    def new(color : tuple):
        return Color(color[0], color[1], color[2], color[3] if len(color) > 3 else 255)

    red : int
    green : int
    blue : int
    alpha : int = 255

@dataclass
class BiomeInfo :
    grass : Color
    water : Color
    foliage : Color
    dry_foliage : Color

@dataclass
class StateInfo :
    full_state : dict
    state : dict
    color : Color
    tint : int
    id : int

@dataclass
class BlockInfo :
    states : list

    def remove_id(self, id : int):
        for state in self.states :
            if state.id == id :
                self.states.remove(state)
                break

    def has_id(self, id : int):
        for state in self.states :
            if state.id == id :
                return True

        return False


def load_blockstate (name : str, blockstates : Path=None) -> dict :
    if name not in BLOCKSTATE_CACHE and blockstates is not None:
        with open(blockstates / (name + ".json"), "r") as file :
            blockstate = json.load(file)
            BLOCKSTATE_CACHE[name] = blockstate
            return blockstate
    return BLOCKSTATE_CACHE[name]

def load_model(name : str, models : Path=None) -> dict :
    if name not in MODELS_CACHE and models is not None:
        with open(models / (name + ".json"), "r") as file :
            model = json.load(file)
            MODELS_CACHE[name] = model
            return model
    return MODELS_CACHE[name]

def load_texture(name : str, textures : Path=None) -> Optional[Image.Image] :
    path = textures / (name + ".png")
    if str(path) not in TEXTURES_CACHE and textures is not None:
        try :
            texture = Image.open(path)
            TEXTURES_CACHE[str(path)] = texture
        except Exception :
            return None

    return TEXTURES_CACHE[str(path)]

# this is probably the most fragile part of the script, we should 100% just build the model out and render it, but whatev
def get_top_texture(textures : dict) -> Optional[str] :
    for key in ["up", "top", "end", "all", "side", "particle", "cross"] : # order matters here
        if key in textures :
            return textures[key].replace("#", "").split("/")[-1]

    return None

def resolve_inheritance(model : dict, models : Path) -> dict:
    resolved = model.copy()

    if "parent" in model : # has inheritance
        parent_name = model["parent"].split("/")[-1]
        parent = resolve_inheritance(load_model(parent_name, models), models)

        for key, value in parent.items() :
            if key not in resolved :
                resolved[key] = value
            elif key == "textures" and isinstance(value, dict) :
                merged_textures = value.copy()
                merged_textures.update(resolved[key])
                resolved[key] = merged_textures

    return resolved


def get_model_name(blockstate : dict, state : dict) -> Optional[str] :
    if "variants" in blockstate :
        variants : dict = blockstate["variants"]

        if "" in variants : # only one variant
            if isinstance(variants[""], list):
                return variants[""][0]["model"].split("/")[-1]
            else :
                return variants[""]["model"].split("/")[-1]
        else :
            allowed_properties = [property.split("=")[0] for property in str(list(variants.keys())[0]).split(",")]

            properties = []
            for property, property_value in sorted(state["properties"].items()) :
                if property in allowed_properties :
                    properties.append(f"{property}={property_value}")

            variant_key = ",".join(properties)
            models = variants[variant_key]
            if isinstance(models, list) :
                return models[0]["model"].split("/")[-1]
            else :
                return models["model"].split("/")[-1]


    elif "multipart" in blockstate :
        # could fix this, but really unnecessary
        index = 0
        current_index = 0
        for part in blockstate["multipart"] :
            if "when" not in part:
                index = current_index
                break
            current_index += 1
        model =  blockstate["multipart"][index]["apply"]
        if isinstance(model, list) :
            return model[0]["model"].split("/")[-1]
        else:
            return model["model"].split("/")[-1]

    return None

def crop_uv(texture : Image.Image , uv) -> Image.Image :
    if len(uv) != 4 : # ermmmmmm.... wtflip
        return texture

    # UV coordinates are in texture pixel space
    u1, v1, u2, v2 = uv
    w, h = texture.size

    # Convert to pixel coordinates
    x1 = int(u1 * w / 16)
    y1 = int(v1 * h / 16)
    x2 = int(u2 * w / 16)
    y2 = int(v2 * h / 16)

    # dumb but works for our purrrrpose
    if x2 < x1 :
        temp = x2
        x2 = x1
        x1 = temp

    if y2 < y1 :
        temp = y2
        y2 = y1
        y1 = temp

    return texture.crop((x1, y1, x2, y2))

# this script is getting real messy
def find_first_tag(data, target_key):
    if isinstance(data, dict):
        if target_key in data:
            return data[target_key]

        for value in data.values():
            result = find_first_tag(value, target_key)
            if result is not None:
                return result

    elif isinstance(data, list):
        for item in data:
            result = find_first_tag(item, target_key)
            if result is not None:
                return result

    return None

def generate_colors(blocks: dict, client : Path) -> dict :
    output : dict = {} # dict of <str (name), Block (list of blockstates)>
    assets = client / "assets" / "minecraft"

    blockstates = assets / "blockstates"
    models = assets / "models" / "block"

    for name, data in blocks.items() :
        for state in data["states"] :
            blockstate = load_blockstate(str(name).split(":")[1], blockstates)

            model_name = get_model_name(blockstate, state)
            if not model_name:
                continue # couldn't find model

            model = load_model(model_name, models)

            model = resolve_inheritance(model, models)

            tint_index = find_first_tag(model, "tintindex")
            if tint_index is None :
                tint_index = -1

            textures = model.get("textures", {}) # should never default (I've checked!!!)

            textures_path = assets / "textures" / "block"

            image = Image.new("RGBA", (16, 16), (0, 0, 0, 0))

            if "elements" in model :

                found_texture = False # I hate vars like this omg

                for element in model["elements"] :
                    if "faces" not in element or "up" not in element["faces"] : # cringe alert!!!!
                        continue

                    face : dict = element["faces"]["up"]

                    texture_key = face.get("texture", "").replace("#", "")

                    if texture_key not in textures:
                        continue

                    texture_name = textures[texture_key].replace("#", "")
                    if texture_name in textures:
                        texture_name = textures[texture_name]

                    texture_name = texture_name.split("/")[-1]

                    texture = load_texture(texture_name, textures_path)

                    uv = face.get("uv", [0, 0, 16, 16]) # 16 will get scaled to the image size! dw if tex is larger!

                    cropped = crop_uv(texture, uv)

                    from_pos = element.get("from", [0, 0, 0])
                    to_pos = element.get("to", [16, 16, 16])

                    x1, z1 = int(from_pos[0]), int(from_pos[2])
                    x2, z2 = int(to_pos[0]), int(to_pos[2])

                    if x2 > x1 and z2 > z1:
                        resized = cropped.resize((x2 - x1, z2 - z1))
                        image.paste(resized, (x1, z1), resized.convert("RGBA"))

                    found_texture = True

                if not found_texture:
                    texture_key = get_top_texture(textures)
                    if texture_key in textures :
                        texture_name = textures[texture_key].split("/")[-1]
                    else :
                        texture_name = texture_key

                    image = load_texture(texture_name, textures_path)
            else :
                top_texture = get_top_texture(textures)
                if top_texture:
                    image = load_texture(top_texture, textures_path)
                else : continue

            total_r = total_g = total_b = total_a = 0
            pixel_count = 0

            if not image :
                continue

            image = image.convert("RGBA")

            for r, g, b, a in image.getdata():
                if a > 0:  # Not transparent
                    total_r += r
                    total_g += g
                    total_b += b
                    total_a += a
                    pixel_count += 1

            if pixel_count == 0 : #erm whattheflip
                continue

            color = (total_r // pixel_count, total_g // pixel_count, total_b // pixel_count, total_a // pixel_count)

            if name not in output :
                output[name] = BlockInfo([])

            output[name].states.append(StateInfo(state["properties"] if "properties" in state else {}, None, Color(color[0], color[1], color[2], color[3]), tint_index, state["id"]))
    return output

def generate_state_lookup(blocks : dict, colors : dict) -> str :
    for name, data in blocks.items() :
        if name not in colors :
            colors[name] = BlockInfo([StateInfo({}, {}, Color(0, 0, 0, 0), -1, None)])
        info = colors[name]

        for original_state in data["states"] :
            blockstate = load_blockstate(name.split(":")[1])
            if not blockstate:
                info.states.remove(next(filter(lambda x: x.id == original_state["id"], info.states), None))
                continue

            if "multipart" in blockstate :
                if not info.has_id(original_state["id"]) :
                    continue
                info.states = info.states[:1]
                info.states[0].state = {}
                break

            if "variants" not in blockstate :
                info.remove_id(original_state["id"])
                continue

            variants = blockstate["variants"]

            if "" in variants :
                if not info.has_id(original_state["id"]) :
                    continue
                info.states = info.states[:1]
                info.states[0].state = {}
                break

        if len(info.states) > 0 : # try to compress properties
            all_properties = info.states[0].full_state.keys()

            def can_distinguish_cases(subset : set) :
                # Group by the selected properties
                groups = {}
                for state in info.states:

                    reduced_key = tuple(sorted((prop, state.full_state[prop]) for prop in subset))

                    if reduced_key not in groups:
                        groups[reduced_key] = set()
                    groups[reduced_key].add((state.color.red, state.color.green, state.color.blue, state.color.alpha, state.tint))

                # Check if any group has multiple different values
                for group_values in groups.values():
                    if len(group_values) > 1:
                        return False

                return True

            # really inefficient, but whatever cuz build time!!!!
            for size in range(0, len(all_properties) + 1) :
                for subset in combinations(all_properties, size) :
                    if can_distinguish_cases(subset) :
                        states = []
                        used_keys = []
                        for state in info.states :
                            new_key = {property : state.full_state[property] for property in subset}
                            if new_key in used_keys :
                                continue
                            used_keys.append(new_key)
                            states.append(StateInfo(state.full_state, new_key, state.color, state.tint, state.id))

                        info.states = states
                        break
                else :
                    continue
                break

        if len(info.states) == 0 :
            info.states.append(StateInfo({}, {}, Color(0, 0, 0, 0), -1, None))

    # I hate this so much but I can't bring myself to make some nasty string builder situation and the conversion only works for this "type" so I don't wanna make a generic dict to map function
    return ",\n".join([f"{{\"{name.split(":")[1]}\","
                            f"{{"
                                f"{",\n".join([f"{{nbt::tag_compound{{{",".join([f"{{\"{property}\", \"{property_value}\"}}" for property, property_value in state.state.items()])}}},"
                                                    f"{{"
                                                        f"{{{state.color.red},{state.color.green},{state.color.blue},{state.color.alpha}}},"
                                                        f"{state.tint}"
                                                    f"}}"
                                               f"}}" for state in block.states])}"
                            f"}}"
                       f"}}" for name, block in colors.items()])

def get_color (color : int | str) -> Color :
    if isinstance(color, str) :
        color = int(color[1:], 16)

    a = color >> 24
    r = color >> 16 & 0xFF
    g = color >> 8 & 0xFF
    b = color & 0xFF
    return Color(r, g, b, a)

def get_biome_colors(biome : dict, grass : Image.Image, foliage : Image.Image, dry_foliage : Image.Image) -> BiomeInfo :
    effects = biome["effects"]
    water_color = get_color(effects["water_color"])

    temperature = (np.clip(biome["temperature"], 0.0, 1.0))
    humidity = (1.0 - (np.clip(biome["downfall"], 0.0, 1.0)) * temperature) * 255
    temperature = (1.0 - temperature) * 255

    if "grass_color" in effects :
        grass_color = get_color(effects["grass_color"])
    else:
        grass_color = Color.new(grass.getpixel((temperature, humidity)))
    if "foliage_color" in effects :
        foliage_color = get_color(effects["foliage_color"])
    else:
        foliage_color = Color.new(foliage.getpixel((temperature, humidity)))
    if "dry_foliage_color" in effects :
        dry_foliage_color = get_color(effects["dry_foliage_color"])
    else:
        dry_foliage_color = Color.new(dry_foliage.getpixel((temperature, humidity)))

    return BiomeInfo(grass_color, water_color, foliage_color, dry_foliage_color)

def generate_biome_colors(client : Path) -> str :
    biome_dir = client / "data" / "minecraft" / "worldgen" / "biome"
    biome_files = [file for file in biome_dir.glob("**/*") if file.is_file()]

    biomes = []

    colormaps = client / "assets" / "minecraft" / "textures" / "colormap"

    for biome_file in biome_files :
        with open(biome_file, "r") as file:
            data = json.load(file)
        biome_colors = get_biome_colors(data, load_texture("grass", colormaps).convert("RGB"), load_texture("foliage", colormaps).convert("RGB"), load_texture("dry_foliage", colormaps).convert("RGB"))
        biomes.append((biome_file.name.split(".")[0], biome_colors))

    return ",\n".join([f"{{\"{biome}\",{NAMESPACE}::BiomeColors{{"
                            f"{",".join([f"{{{color.red},{color.green},{color.blue},{color.alpha}}}" for color in [colors.grass, colors.water, colors.foliage, colors.dry_foliage]])}"
                       f"}}}}" for biome, colors in biomes])

def generate_lookups(file_names : Path, blocks : dict, colors : dict, biomes : str) -> dict:
    output = []
    for name, data in blocks.items() :
        for state in data["states"] :
            if state["id"] not in colors :
                output.append((state["id"], (name.split(":")[-1], {}, (0, 0, 0, 0), -1)))
                continue
            output.append((state["id"], (name.split(":")[-1], state["properties"] if "properties" in state else {}, colors[state["id"]][0] if state["id"] in colors else (0, 0, 0, 0), colors[state["id"]][1] if state["id"] in colors else -1)))

    output = sorted(output, key=lambda v: v[0])
    for i in range(output[-1][0] + 1) :
        if output[i][0] != i :
            output.insert(i, (i, ()))

    output_split = {}

    source = f"""#include \"xaero/lookups/{file_names}.hpp\"
#include \"xaero/types/LookupTypes.hpp\"
#include <nbt_tags.h>

[[maybe_unused]] const {NAMESPACE}::{STATE_TYPE} {NAMESPACE}::{STATE_NAME} = {{
{generate_state_lookup(blocks, colors)}
}};

[[maybe_unused]] const {NAMESPACE}::{BIOME_TYPE} {NAMESPACE}::{BIOME_NAME} = {{
{biomes}
}};
"""

    output_split[f"{file_names}"] = source
    return output_split

def main() :
    parser = argparse.ArgumentParser(description="Generating lookup headers")
    parser.add_argument("--client", required=True)
    parser.add_argument("--reports", required=True)
    parser.add_argument("--output_dir", required=True)
    parser.add_argument("--file_names", required=True)

    args = parser.parse_args()

    with open(Path(args.reports) / "blocks.json", "r") as file :
        blocks = json.load(file)

    manifest = []

    colors = generate_colors(blocks, Path(args.client))
    state_id_split = generate_lookups(args.file_names, blocks, colors, generate_biome_colors(Path(args.client)))
    output = Path(args.output_dir)

    chunk_output_dir = output / "src" / "lookups"
    chunk_output_dir.mkdir(exist_ok=True, parents=True)
    for name, chunk in state_id_split.items() :
        path = chunk_output_dir / (name + ".cpp")
        manifest.append(str(path.as_posix()))
        with open(path, "w") as file:
            file.write(chunk)

    with open(output / "manifest.txt", "w") as file : # I think this is the right term... it's internal anyways
        file.write("\n".join(manifest))

if __name__ == "__main__":
    main()