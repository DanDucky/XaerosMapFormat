#include "../include/xaero/Parser.hpp"

#include <functional>

#include "../include/xaero/util/RegionImage.hpp"
#include "util/ByteInputStream.hpp"
#include "xaero/util/Region.hpp"
#include <nbt_tags.h>

#include "xaero/util/IndexableView.hpp"

namespace xaero {

    static void convertNBT(std::unique_ptr<nbt::tag_compound>& nbt, const std::pair<std::int16_t, std::int16_t>& version) {
        if (version.first == 1) {
            const auto name = nbt->at("Name").as<nbt::tag_string>().get();
            static const std::map<std::string_view, std::string_view> convert {
                {"minecraft:stone_slab", "minecraft:smooth_stone_slab"},
                {"minecraft:sign", "minecraft:oak_sign"},
                {"minecraft:wall_sign", "minecraft:oak_wall_sign"}};
            const auto fixed = convert.find(name);
            nbt->put("Name", (fixed != convert.end() ? fixed->second : name).data());
        }

        if (version.first < 3) {
            const auto name = nbt->at("Name").as<nbt::tag_string>().get();
            static const auto wallFix = [](std::unique_ptr<nbt::tag_compound>& tag) -> void {
                auto& properties = tag->at("Properties").as<nbt::tag_compound>();
                properties.put("north", properties.at("north").as<nbt::tag_string>().get() == "true" ? "low" : "none");
                properties.put("south", properties.at("south").as<nbt::tag_string>().get() == "true" ? "low" : "none");
                properties.put("east", properties.at("east").as<nbt::tag_string>().get() == "true" ? "low" : "none");
                properties.put("west", properties.at("west").as<nbt::tag_string>().get() == "true" ? "low" : "none");
            };
            static const std::map<std::string_view, std::function<void(std::unique_ptr<nbt::tag_compound>&)>> convert {
                {"minecraft:jigsaw", [](std::unique_ptr<nbt::tag_compound>& tag) {
                    auto& properties = tag->at("Properties").as<nbt::tag_compound>();
                    const auto facing = properties.at("facing").as<nbt::tag_string>().get();
                    properties.erase("facing");
                    static const std::map<std::string_view, std::string_view> convert {
                        {"", "north_up"},
                        {"down", "down_south"},
                        {"up", "up_north"},
                        {"north", "north_up"},
                        {"south", "south_up"},
                        {"west", "west_up"},
                        {"east", "east_up"}
                    };

                    properties.put("orientation", convert.at(facing).data());
                }},
                {"minecraft:redstone_wire", [](std::unique_ptr<nbt::tag_compound>& tag) {
                    auto& properties = tag->at("Properties").as<nbt::tag_compound>();
                    auto north = properties.at("north").as<nbt::tag_string>().get();
                    auto south = properties.at("south").as<nbt::tag_string>().get();
                    auto east = properties.at("east").as<nbt::tag_string>().get();
                    auto west = properties.at("west").as<nbt::tag_string>().get();

                    properties.put("north",
                        north.empty() ? (west.empty() && east.empty() ? "side" : "none") : north);
                    properties.put("south",
                        south.empty() ? (west.empty() && east.empty() ? "side" : "none") : south);
                    properties.put("east",
                        east.empty() ? (north.empty() && south.empty() ? "side" : "none") : east);
                    properties.put("west",
                        west.empty() ? (north.empty() && south.empty() ? "side" : "none") : west);
                }},
                {"minecraft:andesite_wall", wallFix},
                {"minecraft:brick_wall", wallFix},
                {"minecraft:cobblestone_wall", wallFix},
                {"minecraft:diorite_wall", wallFix},
                {"minecraft:end_stone_brick_wall", wallFix},
                {"minecraft:granite_wall", wallFix},
                {"minecraft:mossy_cobblestone_wall", wallFix},
                {"minecraft:mossy_stone_brick_wall", wallFix},
                {"minecraft:nether_brick_wall", wallFix},
                {"minecraft:prismarine_wall", wallFix},
                {"minecraft:red_nether_brick_wall", wallFix},
                {"minecraft:red_sandstone_wall", wallFix},
                {"minecraft:sandstone_wall", wallFix},
                {"minecraft:stone_brick_wall", wallFix}
            };

            if (const auto converter = convert.find(name);
                converter != convert.end()) {

                converter->second(nbt);
            }
        }

        if (version.first < 5) {
            const auto name = nbt->at("Name").as<nbt::tag_string>().get();
            static const std::map<std::string_view, std::function<void(std::unique_ptr<nbt::tag_compound>&)>> convert {
                {"minecraft:cauldron",  [](std::unique_ptr<nbt::tag_compound>& tag) {
                    auto& properties = tag->at("Properties").as<nbt::tag_compound>();
                    if (properties.size() == 0) return;

                    if (!properties.has_key("level") || tag->at("level").as<nbt::tag_string>().get() == "0") {
                        tag->erase("Properties");
                    } else {
                        tag->put("Name", "minecraft:water_cauldron");
                    }
                }},
                {"minecraft:grass_path",  [](std::unique_ptr<nbt::tag_compound>& tag) {
                    tag->put("Name", "minecraft:dirt_path");
                }}
            };

            if (const auto converter = convert.find(name);
                converter != convert.end()) {

                converter->second(nbt);
            }
        }

    }

    Region Parser::parseRegion(std::istream &data) {
        ByteInputStream stream(data);
        Region output;

        std::vector<std::shared_ptr<const Region::TileChunk::Chunk::Pixel::BlockState>> palette{};

        const bool hasFullVersion = stream.peekNext<std::uint8_t>() == 255;
        std::pair<std::int16_t, std::int16_t> version;
        bool is115not114 = false;
        if (hasFullVersion) {
            stream.skip(1);
            version.first = stream.getNext<std::int16_t>();
            version.second = stream.getNext<std::int16_t>();
            if (version.first == 2 && version.second >= 5) {
                is115not114 = stream.getNext<std::uint8_t>() == 1; // idk what this is for tbh
            }

            if (version.first >= 6 && version.second > 8) {
                // too new... idk what to do here
            }
        }

        for (std::uint8_t tileIndex = 0; tileIndex < 8*8; tileIndex++) {
            if (stream.eof()) break;
            if (stream.peekNext<std::int8_t>() == -1) {
                break;
            }

            Region::TileChunk& tile = output[stream.getNextBits(4)][stream.getNextBits(4)];
            tile.allocateChunks();
            for (auto& chunkRow : *tile.chunks) {
                for (auto& chunk : chunkRow) {
                    if (stream.peekNext<int32_t>() != -1) {
                        chunk.allocateColumns();

                        for (auto& pixelRow : *chunk.columns) {
                            for (auto& pixel : pixelRow) {
                                const bool isNotGrass = stream.getNextBits(1);
                                const bool hasOverlays = stream.getNextBits(1);
                                pixel.colorType = stream.getNextBits(2);
                                bool hasSlope = false;
                                if (version.second == 2) {
                                    hasSlope = stream.getNextBits(1);
                                }
                                stream.skipBits(1); // idk why this is skipped
                                const bool heightInParameters = !stream.getNextBits(1);
                                stream.skipToNextByte();
                                pixel.light = stream.getNextBits(4);
                                if (heightInParameters) {
                                    pixel.height = stream.getNextBits(8);
                                }
                                const bool hasBiome = stream.getNextBits(1);
                                const bool newStatePaletteEntry = stream.getNextBits(1);
                                const bool newBiomePaletteEntry = stream.getNextBits(1);
                                const bool biomeAsInt = stream.getNextBits(1);
                                const bool topHeightAndHeightDontMatch = version.second >= 4 ? stream.getNextBits(1) : false;
                                if (heightInParameters) {
                                    pixel.height |= stream.getNextBits(3) << 8;
                                    // converting 12 bit signed to 16 bit signed
                                    pixel.height &= 0x0FFF; // mask out garbage
                                    if (pixel.height & 0x8000) { // check if negative, then flip the remaining bits
                                        pixel.height |= 0xF000;
                                    }
                                }
                                stream.skipToNextByte(); // done with "parameters"

                                if (isNotGrass) {
                                    if (version.first == 0) { // old format, state is a state id
                                        const auto state = stream.getNext<int32_t>();
                                        pixel.state = state;
                                    } else {
                                        if (newStatePaletteEntry) {
                                            auto nbtStream = nbt::io::stream_reader(stream.getStream());

                                            auto nbt = nbtStream.read_compound();

                                            if (version.first < 6) {
                                                convertNBT(nbt.second, version); // make it up to date pls !
                                            }
                                            palette.emplace_back(std::make_shared<const Region::TileChunk::Chunk::Pixel::BlockState>(std::move(nbt.first), std::move(*nbt.second))); // copy the stupid compound tag because why is that a ptr
                                            pixel.state = palette.back();
                                        } else {
                                            const auto paletteIndex = stream.getNext<int32_t>();
                                            pixel.state = palette[paletteIndex];
                                        }
                                    }
                                } else { // grass
                                    pixel.state = 9 /* snowless grass block */;
                                }

                                if (!heightInParameters) {
                                    pixel.height = stream.getNext<std::uint8_t>();
                                }

                                if (topHeightAndHeightDontMatch) {
                                    pixel.topHeight = stream.getNext<std::uint8_t>();
                                }

                                if (hasOverlays) {
                                    const auto size = stream.getNext<std::uint8_t>();

                                    pixel.overlays.reserve(size);

                                    for (auto& overlay : pixel.overlays) {
                                        const bool isWater = !stream.getNextBits(1);
                                        const bool legacyOpacity = stream.getNextBits(1);
                                        const bool customColor = stream.getNextBits(1);
                                        const bool hasOpacity = stream.getNextBits(1);
                                        overlay.light = stream.getNextBits(4);
                                        std::uint8_t colorType = 0;

                                        if ( version.second < 5 || version.first <= 2 && !is115not114 ) {
                                            colorType = stream.getNextBits(2);
                                        } else {
                                            stream.skipBits(2);
                                        }

                                        const bool newOverlayStatePaletteEntry = stream.getNextBits(1);

                                        if (version.second >= 8) {
                                            overlay.opacity = stream.getNextBits(4);
                                        }

                                        stream.skip(3); // should skip past parameter int

                                        if (isWater) {
                                            overlay.state = 86; // default water state
                                        } else {
                                            if (version.first == 0) {
                                                overlay.state = stream.getNext<std::int32_t>();
                                            } else {
                                                if (newOverlayStatePaletteEntry) {
                                                    auto nbtStream = nbt::io::stream_reader(stream.getStream());

                                                    const auto nbt = nbtStream.read_compound();

                                                    palette.emplace_back(std::make_shared<const Region::TileChunk::Chunk::Pixel::BlockState>(std::move(nbt.first), std::move(*nbt.second)));
                                                    overlay.state = palette.back();
                                                } else {
                                                    overlay.state = palette[stream.getNext<std::int32_t>()];
                                                }
                                            }
                                        }

                                        // so I know this is just skipping "important" info, but this is exactly what Xaero does and I can't be bothered to fix it

                                        if (version.second < 1 && legacyOpacity) {
                                            stream.skip(4);
                                        }

                                        if (colorType == 2 || customColor) {
                                            stream.skip(4);
                                        }

                                        if (version.second < 8 && hasOpacity) {
                                            overlay.opacity = stream.getNext<std::int32_t>();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return output;
    }

    Region Parser::parseRegion(const std::string &data) {
        std::istringstream stringStream(data);
        return parseRegion(stringStream);
    }

    void Parser::addRegion(const std::filesystem::path &file) {
    }

    void Parser::addRegion(const std::string &data) {
    }

    void Parser::addRegion(const std::string_view &data) {
    }

    void Parser::clearRegions() {
    }

    void Parser::removeRegion(int x, int z) {
    }

    RegionImage Parser::generateImage(int x, int z) const {
        return {};
    }

    std::list<RegionImage> Parser::generateImages() const {
        return {};
    }

} // xaero