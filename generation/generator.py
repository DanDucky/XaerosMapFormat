from PIL import Image
import argparse
import json
from pathlib import Path
from typing import Optional

NAMESPACE = "xaero"
STATE_TYPE = "StateLookup"
STATE_NAME = "defaultStateLookup"
STATE_COLOR_TYPE = "StateColorLookup"
STATE_COLOR_NAME = "defaultStateColorLookup"

MODELS_CACHE = {}
TEXTURES_CACHE = {}

def load_model(name : str, models : Path) -> dict :
    if name not in MODELS_CACHE :
        with open(models / (name + ".json"), "r") as file :
            model = json.load(file)
            MODELS_CACHE[name] = model
            return model
    return MODELS_CACHE[name]

def load_texture(name : str, textures : Path) -> Optional[Image.Image] :
    if name not in TEXTURES_CACHE :
        try :
            texture = Image.open(textures / (name + ".png"))
            TEXTURES_CACHE[name] = texture
        except Exception :
            return None

    return TEXTURES_CACHE[name]

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
        for part in blockstate["multipart"] :
            if "when" not in part:
                model = part["apply"]
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

def generate_colors(blocks: dict, client : Path) -> dict :
    output : dict = {}
    assets = client / "assets" / "minecraft"

    blockstates = assets / "blockstates"
    models = assets / "models" / "block"

    for name, data in blocks.items() :
        for state in data["states"] :
            with open(blockstates / (str(name).split(":")[1] + ".json"), "r") as file :
                blockstate = json.load(file)

            model_name = get_model_name(blockstate, state)
            if not model_name:
                continue # couldn't find model

            model = load_model(model_name, models)

            model = resolve_inheritance(model, models)

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

                    if not texture :
                        continue # ermmmm cringe avenue???? next exit please!!!!

                    uv = face.get("uv", [0, 0, 16, 16]) # 16 will get scaled to the image size! dw if tex is larger!

                    cropped = crop_uv(texture, uv)

                    from_pos = element.get("from", [0, 0, 0])
                    to_pos = element.get("to", [16, 16, 16])

                    x1, z1 = int(from_pos[0]), int(from_pos[2])
                    x2, z2 = int(to_pos[0]), int(to_pos[2])

                    if x2 > x1 and z2 > z1:
                        resized = cropped.resize((x2 - x1, z2 - z1))
                        image.paste(resized, (x1, z1))

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

            total_r = total_g = total_b = 0
            pixel_count = 0

            if not image :
                continue

            image = image.convert("RGBA")

            for r, g, b, a in image.getdata():
                if a > 0:  # Not transparent
                    total_r += r
                    total_g += g
                    total_b += b
                    pixel_count += 1

            if pixel_count == 0 : #erm whattheflip
                continue

            color = (total_r // pixel_count, total_g // pixel_count, total_b // pixel_count)

            color_int = 0
            color_int |= color[2]
            color_int |= (color[1] << 8) & 0xFF00
            color_int |= (color[0] << 16) & 0xFF0000

            output[state["id"]] = color

    return output

def generate_header() -> str :
    header = f"""
#pragma once

namespace {NAMESPACE} {{
    static const {STATE_TYPE} {STATE_NAME};
    static const {STATE_COLOR_TYPE} {STATE_COLOR_NAME};
}}
    """

    return header

def generate_state_lookup(blocks : dict) -> str :
    output : str = ""
    for name, data in blocks.items() :
        output += f"{{ {str(name).split(":")[1]}, {{"
        for state in data["states"] :

            output += (f"{{nbt::tag_compound{{{str([{property : value} for property, value in state["properties"].items()]).replace(":", ",")[1:-1]}}}, {{{state["id"]}, "
                       f"xaero::RegionImage::Pixel{{ 0, 0, 0, 0 }} }} }},")

        output = output[:-1] # remove comma
        output += "}},\n"

    return output[:-1]

def generate_source(file_names : str, blocks) :
    source = f""" 
#include \"../../include/lookups/{file_names}.hpp\"
#include <nbt_tags.h>

static const {STATE_TYPE} {NAMESPACE}::{STATE_NAME} = {{
    {generate_state_lookup(blocks)}
}};

"""

def main() :
    parser = argparse.ArgumentParser(description="Generating lookup headers")
    parser.add_argument("--client", required=True)
    parser.add_argument("--reports", required=True)
    parser.add_argument("--output_dir", required=True)
    parser.add_argument("--file_names", required=True)

    args = parser.parse_args()

    with open(Path(args.reports) / "blocks.json", "r") as file :
        blocks = json.load(file)

    colors = generate_colors(blocks, Path(args.client))
    print(colors)
    # header = generate_header()
    # source = generate_source(args.file_names, blocks)

if __name__ == "__main__":
    main()