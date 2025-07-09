#include "../include/xaero/Parser.hpp"

#include <functional>

#include "../include/xaero/util/RegionImage.hpp"
#include "util/ByteInputStream.hpp"
#include "xaero/util/Region.hpp"
#include <nbt_tags.h>

namespace xaero {
    Region parseRegion(ByteInputStream &stream) {
        Region output;

        bool hasFullVersion = stream.peekNext<std::uint8_t>() == 255;
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
            if (stream.peekNext<std::uint8_t>() == -1) [[unlikely]] {
                break;
            }

            Region::TileChunk& tile = output[stream.getNextBits(4)][stream.getNextBits(4)];
            tile.allocateChunks();
            for (auto& chunkRow : *tile.chunks) {
                for (auto& chunk : chunkRow) {
                    if (stream.peekNext<int32_t>() != -1) [[likely]] {
                        chunk.isVoid = false;
                        chunk.allocateColumns();

                        for (auto& pixelRow : *chunk.columns) {
                            for (auto& pixel : pixelRow) {
                                pixel.isNotGrass = stream.getNextBits(1);
                                pixel.hasOverlays = stream.getNextBits(1);
                                pixel.colorType = stream.getNextBits(2);
                                if (version.second == 2) {
                                    pixel.hasSlope = stream.getNextBits(1);
                                }
                                stream.skipBits(1); // idk why this is skipped
                                bool heightInParameters = !stream.getNextBits(1);
                                stream.skipToNextByte();
                                pixel.light = stream.getNextBits(4);
                                if (heightInParameters) {
                                    pixel.height = stream.getNextBits(8);
                                }
                                pixel.hasBiome = stream.getNextBits(1);
                                pixel.newStatePalette = stream.getNextBits(1);
                                pixel.newBiomePalette = stream.getNextBits(1);
                                pixel.biomeAsInt = stream.getNextBits(1);
                                pixel.topHeightAndHeightDontMatch = version.second >= 4 ? stream.getNextBits(1) : false;
                                if (heightInParameters) {
                                    pixel.height |= stream.getNextBits(3) << 8;
                                    // converting 12 bit signed to 16 bit signed
                                    pixel.height &= 0x0FFF; // mask out garbage
                                    if (pixel.height & 0x8000) { // check if negative, then flip the remaining bits
                                        pixel.height |= 0xF000;
                                    }
                                }
                                stream.skipToNextByte(); // done with "parameters"

                                if (pixel.isNotGrass) {
                                    if (version.first == 0) { // old format
                                        const int32_t state = stream.getNext<int32_t>();
                                    } else {
                                        if (pixel.newStatePalette) {
                                            auto nbtStream = nbt::io::stream_reader(stream.getStream());
                                            // const auto type = nbtStream.read_type();
                                            // if (type != nbt::tag_type::Compound) throw new std::exception; // must be a compound tag, this is an error!

                                            auto data = nbtStream.read_compound();

                                            if (version.first < 6) {
                                                if (version.first == 1) {
                                                    const auto name = data.second->at("Name").as<nbt::tag_string>().get();
                                                    static const std::map<std::string_view, std::string_view> convert {
                                                        {"minecraft:stone_slab", "minecraft:smooth_stone_slab"},
                                                        {"minecraft:sign", "minecraft:oak_sign"},
                                                        {"minecraft:wall_sign", "minecraft:oak_wall_sign"}};
                                                    const auto fixed = convert.find(name);
                                                    data.second->put("Name", (fixed != convert.end() ? fixed->second : name).data());
                                                }

                                                if (version.first < 3) {
                                                    const auto name = data.second->at("Name").as<nbt::tag_string>().get();
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

                                                        converter->second(data.second);
                                                    }
                                                }

                                                if (version.first < 5) {
                                                    const auto name = data.second->at("Name").as<nbt::tag_string>().get();
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

                                                        converter->second(data.second);
                                                    }
                                                }
                                            }
                                        } else {
                                            const auto paletteIndex = stream.getNext<int32_t>();
                                        }
                                    }
                                } else { // grass

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
        auto stream = ByteInputStream(stringStream);
        return xaero::parseRegion(stream);
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
    }

    std::list<RegionImage> Parser::generateImages() const {
    }

} // xaero