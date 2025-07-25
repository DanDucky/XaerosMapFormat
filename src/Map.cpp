#include "../include/xaero/Map.hpp"

#include <fstream>
#include <functional>
#include <memory>
#include <algorithm>

#include "../include/xaero/types/RegionImage.hpp"
#include "util/ByteInputStream.hpp"
#include "../include/xaero/types/Region.hpp"
#include <nbt_tags.h>

#include "../include/xaero/types/LookupTypes.hpp"
#include "xaero/util/IndexableView.hpp"

// for some reason when I remove this include I get errors, so keep mz_strm
#include <mz_strm.h>
#include <mz.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>
#include <spanstream>

#include "util/ByteOutputStream.hpp"
#include "util/OptionalOwnerPtr.hpp"

namespace xaero {

    enum class ColorType : std::uint8_t { // legacy color type system, not supported by Xaero and not rendered by this
        NONE,
        DEFAULT,
        CUSTOM_COLOR,
        CUSTOM_BIOME
    }; // the names here are GUESSES and probably incorrect, thank Xaero for using magic numbers everywhere and then not providing source code with comments

    static std::string_view fixBiome (const std::string_view& biome) {
        static const std::unordered_map<std::string_view, std::string_view, NameHash, NameEquals> lookup {
            {"badlands_plateau", "badlands"},
            {"bamboo_jungle_hills", "bamboo_jungle"},
            {"birch_forest_hills", "birch_forest"},
            {"dark_forest_hills", "dark_forest"},
            {"desert_hills", "desert"},
            {"desert_lakes", "desert"},
            {"giant_spruce_taiga_hills", "old_growth_spruce_taiga"},
            {"giant_spruce_taiga", "old_growth_spruce_taiga"},
            {"giant_tree_taiga_hills", "old_growth_pine_taiga"},
            {"giant_tree_taiga", "old_growth_pine_taiga"},
            {"gravelly_mountains", "windswept_gravelly_hills"},
            {"jungle_edge", "sparse_jungle"},
            {"jungle_hills", "jungle"},
            {"modified_badlands_plateau", "badlands"},
            {"modified_gravelly_mountains", "windswept_gravelly_hills"},
            {"modified_jungle_edge", "sparse_jungle"},
            {"modified_jungle", "jungle"},
            {"modified_wooded_badlands_plateau", "wooded_badlands"},
            {"mountain_edge", "windswept_hills"},
            {"mountains", "windswept_hills"},
            {"mushroom_field_shore", "mushroom_fields"},
            {"shattered_savanna", "windswept_savanna"},
            {"shattered_savanna_plateau", "windswept_savanna"},
            {"snowy_mountains", "snowy_plains"},
            {"snowy_taiga_hills", "snowy_taiga"},
            {"snowy_taiga_mountains", "snowy_taiga"},
            {"snowy_tundra", "snowy_plains"},
            {"stone_shore", "stony_shore"},
            {"swamp_hills", "swamp"},
            {"taiga_hills", "taiga"},
            {"taiga_mountains", "taiga"},
            {"tall_birch_forest", "old_growth_birch_forest"},
            {"tall_birch_hills", "old_growth_birch_forest"},
            {"wooded_badlands_plateau", "wooded_badlands"},
            {"wooded_hills", "forest"},
            {"wooded_mountains", "windswept_forest"},
            {"lofty_peaks", "jagged_peaks"},
            {"snowcapped_peaks", "frozen_peaks"}
        };

        if (const auto fixed = lookup.find(biome);
            fixed != lookup.end()) {

            return fixed->second;
        }
        return biome;
    }

    static std::string_view getBiomeFromID(const std::uint32_t biomeID) {
        static constexpr std::string_view lookup[] = {
            "ocean","plains","desert","mountains","forest","taiga","swamp","river","nether_wastes","the_end","frozen_ocean","frozen_river","snowy_tundra","snowy_mountains","mushroom_fields","mushroom_field_shore","beach","desert_hills","wooded_hills","taiga_hills","mountain_edge","jungle","jungle_hills","jungle_edge","deep_ocean","stone_shore","snowy_beach","birch_forest","birch_forest_hills","dark_forest","snowy_taiga","snowy_taiga_hills","giant_tree_taiga","giant_tree_taiga_hills","wooded_mountains","savanna","savanna_plateau","badlands","wooded_badlands_plateau","badlands_plateau","small_end_islands","end_midlands","end_highlands","end_barrens","warm_ocean","lukewarm_ocean","cold_ocean","deep_warm_ocean","deep_lukewarm_ocean","deep_cold_ocean","deep_frozen_ocean","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","the_void","","sunflower_plains","desert_lakes","gravelly_mountains","flower_forest","taiga_mountains","swamp_hills","","","","","","ice_spikes","","","","","","","","","modified_jungle","","modified_jungle_edge","","","","tall_birch_forest","tall_birch_hills","dark_forest_hills","snowy_taiga_mountains","","giant_spruce_taiga","giant_spruce_taiga_hills","modified_gravelly_mountains","shattered_savanna","shattered_savanna_plateau","eroded_badlands","modified_wooded_badlands_plateau","modified_badlands_plateau","bamboo_jungle","bamboo_jungle_hills","soul_sand_valley","crimson_forest","warped_forest","basalt_deltas","dripstone_caves","lush_caves","","meadow","grove","snowy_slopes","snowcapped_peaks","lofty_peaks","stony_peaks"
        };

        if (biomeID >= sizeof(lookup) / sizeof(std::string_view) ||
            lookup[biomeID].empty()) {

            return "plains";
        }
        return lookup[biomeID];
    }

    static void convertNBT(std::unique_ptr<nbt::tag_compound>& nbt, const std::int16_t majorVersion) {
        if (majorVersion == 1) {
            const auto name = nbt->at("Name").as<nbt::tag_string>().get();
            static const std::map<std::string_view, std::string_view> convert {
                {"minecraft:stone_slab", "minecraft:smooth_stone_slab"},
                {"minecraft:sign", "minecraft:oak_sign"},
                {"minecraft:wall_sign", "minecraft:oak_wall_sign"}};
            const auto fixed = convert.find(name);
            nbt->put("Name", (fixed != convert.end() ? fixed->second : name).data());
        }

        if (majorVersion < 3) {
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

        if (majorVersion < 5) {
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

    Region Map::parseRegion(std::istream &data) {
        ByteInputStream stream(data);
        Region region;

        /*
         The reason these use std::shared_ptr instead of a non owning ptr or view to some private vector owned by the region
         is because users might want to copy certain chunks or pixels from one region to another, or from one pixel to another,
         and in both cases the underlying data should not change, particularly for the state as that would be a really large copy for the underlying map.
         Also, in destroying the first region you might break other regions, and I don't want that to even be a concern.
         Even though careful programming could fix all of these issues, I really don't think it's worth the risk
         */

        std::vector<std::shared_ptr<BlockState>> statePalette{};
        std::vector<std::shared_ptr<std::string>> biomePalette{};

        const bool hasFullVersion = stream.peekNext<std::uint8_t>() == 255;

        bool is115not114 = false;
        if (hasFullVersion) {
            stream.skip(1);
            region.majorVersion = stream.getNext<std::int16_t>();
            region.minorVersion = stream.getNext<std::int16_t>();
            if (region.majorVersion == 2 && region.minorVersion >= 5) {
                is115not114 = stream.getNext<std::uint8_t>() == 1; // idk what this is for tbh
            }

            if (region.majorVersion > 6 || region.minorVersion > 8) {
                // unrecognized version... return unexpected
            }
        }

        const bool usesColorTypes = region.minorVersion < 5 || region.majorVersion <= 2 && !is115not114;

        for (std::uint8_t tileCount = 0; tileCount < 8*8; tileCount++) { // just for max iterations

            auto coordinates = stream.getNextAsView<std::uint8_t>();
            if (stream.eof()) {
                break;
            }

            Region::TileChunk& tile = region[coordinates.getNextBits(4)][coordinates.getNextBits(4)];
            tile.allocateChunks();
            for (auto& chunkRow : *tile.chunks) {
                for (auto& chunk : chunkRow) {
                    if (stream.peekNext<int32_t>() == -1) {
                        stream.skip(4);
                        continue;
                    }

                    chunk.allocateColumns();

                    for (auto& pixelRow : *chunk.columns) {
                        for (auto& pixel : pixelRow) {
                            auto parameters = stream.getNextAsView<std::uint32_t>();
                            const bool isNotGrass = parameters.getNextBits(1);
                            const bool hasOverlays = parameters.getNextBits(1);
                            const auto colorType = usesColorTypes ? static_cast<ColorType>(parameters.peekNextBits(2)) : ColorType::NONE;

                            parameters.skipBits(2);

                            bool hasSlope = false;
                            if (region.minorVersion == 2) {
                                hasSlope = parameters.getNextBits(1);
                            } else {
                                parameters.skipBits(1);
                            }
                            const bool heightInParameters = !parameters.getNextBits(1);
                            parameters.skipToNextByte();
                            pixel.light = parameters.getNextBits(4);
                            if (heightInParameters) {
                                pixel.height = parameters.getNextBits(8);
                            } else {
                                parameters.skipBits(8);
                            }
                            const bool hasBiome = parameters.getNextBits(1);
                            const bool newStatePaletteEntry = parameters.getNextBits(1);
                            const bool newBiomePaletteEntry = parameters.getNextBits(1);
                            const bool biomeAsInt = parameters.getNextBits(1);
                            const bool topHeightAndHeightDontMatch = region.minorVersion >= 4 ? parameters.getNextBits(1) : false;
                            if (heightInParameters) {
                                // todo this probably breaks on big endian machines
                                pixel.height |= static_cast<std::uint16_t>(parameters.getNextBits(4)) << 8;
                                // converting 12 bit signed to 16 bit signed
                                pixel.height &= 0x0FFF; // mask out garbage
                                if (pixel.height & 0x8000) { // check if negative, then flip the remaining bits
                                    pixel.height |= 0xF000;
                                }
                            }

                            // done with parameters

                            if (isNotGrass) {
                                if (region.majorVersion == 0) { // old format, state is a state id
                                    const auto state = stream.getNext<int32_t>();
                                    pixel.state = state;
                                } else {
                                    if (newStatePaletteEntry) {
                                        auto nbtStream = nbt::io::stream_reader(stream.getStream());

                                        auto nbt = nbtStream.read_compound();

                                        if (region.majorVersion < 6) {
                                            convertNBT(nbt.second, region.majorVersion); // make it up to date pls !
                                        }
                                        statePalette.emplace_back(std::make_shared<BlockState>(std::move(*nbt.second))); // copy the stupid compound tag because why is that a ptr
                                        pixel.state = statePalette.back();
                                    } else {
                                        const auto paletteIndex = stream.getNext<int32_t>();
                                        pixel.state = statePalette[paletteIndex];
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

                                pixel.overlays.resize(size);

                                for (auto& overlay : pixel.overlays) {
                                    auto overlayParameters = stream.getNextAsView<std::uint32_t>();
                                    const bool isWater = !overlayParameters.getNextBits(1);
                                    const bool legacyOpacity = overlayParameters.getNextBits(1);
                                    const bool customColor = overlayParameters.getNextBits(1);
                                    const bool hasOpacity = overlayParameters.getNextBits(1);
                                    overlay.light = overlayParameters.getNextBits(4);
                                    auto overlayColorType = ColorType::NONE;

                                    if (usesColorTypes) {
                                        overlayColorType = static_cast<ColorType>(overlayParameters.getNextBits(2));
                                    } else {
                                        overlayParameters.skipBits(2);
                                    }

                                    const bool newOverlayStatePaletteEntry = overlayParameters.getNextBits(1);

                                    if (region.minorVersion >= 8) {
                                        overlay.opacity = overlayParameters.getNextBits(4);
                                    }

                                    // done with params

                                    if (isWater) {
                                        overlay.state = 86; // default water state
                                    } else {
                                        if (region.majorVersion == 0) {
                                            overlay.state = stream.getNext<std::int32_t>();
                                        } else {
                                            if (newOverlayStatePaletteEntry) {
                                                auto nbtStream = nbt::io::stream_reader(stream.getStream());

                                                const auto nbt = nbtStream.read_compound();

                                                statePalette.emplace_back(std::make_shared<BlockState>(std::move(*nbt.second)));
                                                overlay.state = statePalette.back();
                                            } else {
                                                overlay.state = statePalette[stream.getNext<std::int32_t>()];
                                            }
                                        }
                                    }

                                    // so I know this is just skipping "important" info, but this is exactly what Xaero does and I can't be bothered to fix it

                                    if (region.minorVersion < 1 && legacyOpacity) {
                                        stream.skip(4);
                                    }

                                    if (overlayColorType == ColorType::CUSTOM_COLOR || customColor) {
                                        stream.skip(4);
                                    }

                                    if (region.minorVersion < 8 && hasOpacity) {
                                        overlay.opacity = stream.getNext<std::int32_t>();
                                    }
                                }
                            }

                            if (colorType == ColorType::CUSTOM_BIOME) {
                                stream.skip(4); // custom biome (I think!!!)
                            }

                            if (colorType != ColorType::NONE && colorType != ColorType::CUSTOM_BIOME || hasBiome) {
                                if (region.majorVersion < 4) {
                                    const auto biomeByte = stream.getNext<std::uint8_t>();

                                    // so the old biome id system is completely depreciated in modern mc and modern mc.
                                    // the only IDs which would go past 255 would be modded, and I (obviously) don't include those in the lookup.
                                    // anything beyond 255 or modded will just become minecraft:plains until it's properly overridden by the user

                                    std::uint32_t biomeID;
                                    if (region.minorVersion >= 3 && biomeByte >= 255) { // have to continue reading, one byte isn't enough!
                                        biomeID = stream.getNext<std::int32_t>();
                                    } else {
                                        biomeID = biomeByte;
                                    }
                                    pixel.biome = region.majorVersion < 6 ?
                                        fixBiome(getBiomeFromID(biomeID)) :
                                        getBiomeFromID(biomeID);
                                } else {
                                    if (newBiomePaletteEntry) {
                                        std::string biome;
                                        if (biomeAsInt) {
                                            std::cout << "biome as int palette!\n";
                                            const auto biomeID = stream.getNext<std::int32_t>();
                                            biome = region.majorVersion < 6 ?
                                                fixBiome(getBiomeFromID(biomeID)) :
                                                getBiomeFromID(biomeID);
                                        } else {
                                            const auto biomeName = stream.getNextMUTF(); // mutf is garbage
                                            biome = region.majorVersion < 6 ?
                                                fixBiome(biomeName) :
                                                std::move(biomeName);
                                        }

                                        biomePalette.emplace_back(std::make_shared<std::string>(std::move(biome)));
                                        pixel.biome = biomePalette.back();
                                    } else {
                                        const auto index = stream.getNext<std::uint32_t>();
                                        pixel.biome = biomePalette[index];
                                    }
                                }
                            }

                            if (region.minorVersion == 2 && hasSlope) {
                                pixel.slope = stream.getNext<std::uint8_t>();
                            }
                        }
                    }

                    if (region.minorVersion >= 4) {
                        chunk.chunkInterpretationVersion = stream.getNext<std::int8_t>();
                    }

                    if (region.minorVersion >= 6) { // cave layers stuff... I don't feel like implementing this
                        chunk.caveStart = stream.getNext<std::int32_t>();
                        if (region.minorVersion >= 7) {
                            chunk.caveDepth = stream.getNext<std::int8_t>();
                        }
                    }
                }
            }
        }

        return region;
    }

    inline void serializeRegionImpl(const Region& region, ByteOutputStream& stream, std::function<void(std::size_t)> reserve, const LookupPack& lookups) {
        reserve(5);

        stream.write<std::uint8_t>(255); // has version

        stream.write<std::uint16_t>(6); // "major version"
        stream.write<std::uint16_t>(8); // "minor version"

        std::vector<OptionalOwnerPtr<const BlockState>> statePalette;
        auto findStateInPalette = [&statePalette](const OptionalOwnerPtr<const BlockState>& state) -> std::optional<std::size_t> {
            const auto found = std::ranges::find_if(statePalette.begin(), statePalette.end(), [&state](const OptionalOwnerPtr<const BlockState>& b) -> bool {
                return *state.pointer == *b.pointer;
            });

            if (found == statePalette.end()) {
                return {};
            }
            return found - statePalette.begin();
        };
        std::vector<std::string_view> biomePalette;

        for (std::uint8_t tileX = 0; tileX < 8; tileX++) {
            for (std::uint8_t tileZ = 0; tileZ < 8; tileZ++) {
                if (!region[tileX][tileZ].isPopulated()) continue;

                reserve(4*4*4 + stream.getStream().tellp());

                BitWriter<std::uint8_t> coordinates;
                coordinates.writeNext(tileX, 4);
                coordinates.writeNext(tileZ, 4);

                stream.write(coordinates); // tile chunk coordinates

                const auto& tileChunk = region[tileX][tileZ];

                for (auto& chunkRow : *tileChunk.chunks) {
                    for (auto& chunk : chunkRow) {
                        if (!chunk.isPopulated()) {
                            stream.write<std::int32_t>(-1);
                            continue;
                        }

                        for (auto& pixelRow : *chunk.columns) {
                            for (auto& pixel : pixelRow) {
                                BitWriter<std::uint32_t> parameters;

                                bool isGrass = false;

                                OptionalOwnerPtr<const BlockState> state;

                                if (std::holds_alternative<std::monostate>(pixel.state)) {
                                    state = OptionalOwnerPtr<const BlockState>(&lookups.stateIDLookup[0].value().state, false);
                                } else if (std::holds_alternative<std::int32_t>(pixel.state)) { // get state from id
                                    if (const auto stateID = std::get<std::int32_t>(pixel.state);
                                        stateID == 9 || stateID == 8) {

                                        isGrass = true;
                                    } else {
                                        if (stateID >= lookups.stateIDLookupSize) { // out of bounds...
                                            state = OptionalOwnerPtr<const BlockState>(&lookups.stateIDLookup[0].value().state, false);
                                        } else {
                                            if (const auto statePack = lookups.stateIDLookup[stateID];
                                                !statePack) {

                                                state = OptionalOwnerPtr<const BlockState>(&lookups.stateIDLookup[0].value().state, false);
                                            } else {
                                                state = OptionalOwnerPtr(&statePack.value().state, false);
                                            }
                                        }
                                    }
                                } else {
                                    if (std::holds_alternative<BlockState*>(pixel.state)) {
                                        state = OptionalOwnerPtr<const BlockState>(std::get<BlockState*>(pixel.state), false);
                                    } else if (std::holds_alternative<BlockState>(pixel.state)) {
                                        state = OptionalOwnerPtr<const BlockState>(&std::get<BlockState>(pixel.state), false);
                                    } else if (std::holds_alternative<std::shared_ptr<BlockState>>(pixel.state)) {
                                        state = OptionalOwnerPtr<const BlockState>(std::get<std::shared_ptr<BlockState>>(pixel.state).get(), false);
                                    }

                                    if (state.pointer->name.contains("grass_block")) {
                                        isGrass = true;
                                    }
                                }

                                parameters.writeNext(isGrass, 1);
                                parameters.writeNext(pixel.hasOverlays(), 1);
                                parameters.writeNext(ColorType::NONE, 2);
                                parameters.skip(1); // slope is no longer supported
                                parameters.writeNext(true, 1); // write height separately
                                parameters.skipToNextByte();
                                parameters.writeNext(pixel.light, 4);
                                parameters.skip(8); // height will be inserted later
                                parameters.writeNext(pixel.biome.has_value(), 1);

                                bool stateInPalette = isGrass;
                                std::size_t statePaletteIndex = 0;
                                if (!isGrass) {
                                    const auto found = findStateInPalette(state);

                                    stateInPalette = found.has_value();
                                    if (found.has_value()) {
                                        statePaletteIndex = found.value();
                                    }
                                }

                                parameters.writeNext(!stateInPalette, 1);

                                bool biomeInPalette = !pixel.biome.has_value();
                                std::size_t biomePaletteIndex = 0;
                                std::string_view biome;
                                if (pixel.biome.has_value()) {
                                    if (std::holds_alternative<std::shared_ptr<std::string>>(pixel.biome.value())) {
                                        biome = *std::get<std::shared_ptr<std::string>>(pixel.biome.value());
                                    } else if (std::holds_alternative<std::string>(pixel.biome.value())) {
                                        biome = std::get<std::string>(pixel.biome.value());
                                    } else if (std::holds_alternative<std::string_view>(pixel.biome.value())) {
                                        biome = std::get<std::string_view>(pixel.biome.value());
                                    }

                                    const auto foundSplit = biome.find(':');
                                    if (foundSplit != std::string::npos) {
                                        biome = std::string_view(biome.begin() + foundSplit + 1, biome.end());
                                    }

                                    if (const auto found = std::ranges::find(biomePalette.begin(), biomePalette.end(), biome);
                                        found == biomePalette.end()) {

                                        biomeInPalette = false;
                                    } else {
                                        biomeInPalette = true;
                                        biomePaletteIndex = found - biomePalette.begin();
                                    }
                                }

                                parameters.writeNext(!biomeInPalette, 1);
                                parameters.writeNext(false, 1); // biome is id
                                parameters.writeNext(pixel.topHeight.has_value(), 1);

                                stream.write(parameters); // pixel parameters

                                if (!isGrass) {
                                    if (stateInPalette) {
                                        stream.write<std::uint32_t>(statePaletteIndex);
                                    } else {
                                        nbt::io::stream_writer nbtWriter(stream.getStream());

                                        nbtWriter.write_payload(state.pointer->getNBT());

                                        statePalette.push_back(std::move(state));
                                    }
                                }

                                if (pixel.topHeight.has_value()) {
                                    stream.write<std::int8_t>(pixel.topHeight.value());
                                }

                                if (pixel.hasOverlays()) {
                                    stream.write<std::uint8_t>(pixel.overlays.size());

                                    for (const auto& overlay : pixel.overlays) {
                                        BitWriter<std::uint32_t> overlayParameters;

                                        bool isWater = false;
                                        if (std::holds_alternative<std::monostate>(overlay.state)) {
                                            state = OptionalOwnerPtr<const BlockState>(&lookups.stateIDLookup[0].value().state, false);
                                        } else if (std::holds_alternative<std::int32_t>(overlay.state)) {
                                            if (const auto stateID = std::get<std::int32_t>(overlay.state);
                                                stateID == 86) {

                                                isWater = true;
                                            } else {
                                                if (stateID >= lookups.stateIDLookupSize) {
                                                    state = OptionalOwnerPtr<const BlockState>(&lookups.stateIDLookup[0].value().state, false);
                                                } else {
                                                    if (const auto statePack = lookups.stateIDLookup[stateID];
                                                        !statePack) {

                                                        state = OptionalOwnerPtr<const BlockState>(&lookups.stateIDLookup[0].value().state, false);
                                                    } else {
                                                        state = OptionalOwnerPtr(&statePack.value().state, false);
                                                    }
                                                }
                                            }
                                        } else {
                                            if (std::holds_alternative<BlockState*>(overlay.state)) {
                                                state = OptionalOwnerPtr<const BlockState>(std::get<BlockState*>(overlay.state), false);
                                            } else if (std::holds_alternative<BlockState>(overlay.state)) {
                                                state = OptionalOwnerPtr<const BlockState>(&std::get<BlockState>(overlay.state), false);
                                            } else if (std::holds_alternative<std::shared_ptr<BlockState>>(overlay.state)) {
                                                state = OptionalOwnerPtr<const BlockState>(std::get<std::shared_ptr<BlockState>>(overlay.state).get(), false);
                                            }

                                            if (state.pointer->name.ends_with("water") /* todo this is a bad impl, should strip and check the whole string */) {
                                                isWater = true;
                                            }
                                        }

                                        overlayParameters.writeNext(!isWater, 1);
                                        overlayParameters.writeNext(false, 1); // legacy opacity
                                        overlayParameters.writeNext(false, 1); // custom color (legacy)
                                        overlayParameters.writeNext(overlay.opacity.has_value(), 1);
                                        overlayParameters.writeNext(overlay.light, 4);
                                        overlayParameters.writeNext(ColorType::NONE, 2); // legacy

                                        bool overlayStateInPalette = isWater;
                                        std::size_t overlayStatePaletteIndex = 0;
                                        if (!isWater) {
                                            const auto found = findStateInPalette(state);
                                            overlayStateInPalette = found.has_value();
                                            if (found.has_value()) {
                                                overlayStatePaletteIndex = found.value();
                                            }
                                        }

                                        overlayParameters.writeNext(!overlayStateInPalette, 1);
                                        if (overlay.opacity) {
                                            overlayParameters.writeNext(overlay.opacity.value(), 4);
                                        }
                                        if (!isWater) {
                                            if (overlayStateInPalette) {
                                                stream.write<std::uint32_t>(overlayStatePaletteIndex);
                                            } else {
                                                nbt::io::stream_writer nbtWriter(stream.getStream());

                                                nbtWriter.write_payload(state.pointer->getNBT());

                                                statePalette.push_back(std::move(state));
                                            }
                                        }
                                    }
                                }

                                if (pixel.biome.has_value()) {
                                    if (biomeInPalette) {
                                        stream.write<std::uint32_t>(biomePaletteIndex);
                                    } else {
                                        const std::string biomeFull = std::format("minecraft:{}", biome);
                                        stream.writeMUTF(biomeFull);

                                        biomePalette.push_back(biome);
                                    }
                                }
                            }
                        }

                        stream.write<std::uint8_t>(chunk.chunkInterpretationVersion);
                        stream.write<std::int32_t>(chunk.caveStart);
                        stream.write<std::uint8_t>(chunk.caveDepth);
                    }
                }
            }
        }
    }

    std::string Map::serializeRegion(const Region &region, const LookupPack &lookups) {
        std::string output;
        auto stringStream = std::ostringstream(output);
        ByteOutputStream stream(stringStream);
        serializeRegionImpl(region, stream, [&output](const std::size_t size) {output.reserve(size);}, lookups);
        return output;
    }

    Region Map::parseRegion(const std::filesystem::path &file) {
        auto zipReader = mz_zip_reader_create();
        int32_t error = MZ_OK;
        std::string data;
        mz_zip_file* fileInfo = nullptr;

        // cast seems to be necessary on windows for some reason
        error = mz_zip_reader_open_file(zipReader, reinterpret_cast<const char*>(file.c_str()));
        if (error != MZ_OK) goto cleanup_zip_reader;

        error = mz_zip_reader_locate_entry(zipReader, "region.xaero", false);
        if (error != MZ_OK) goto cleanup_file;

        error = mz_zip_reader_entry_get_info(zipReader, &fileInfo);
        if (error != MZ_OK || !fileInfo) goto cleanup_file;

        data.resize(fileInfo->uncompressed_size);

        error = mz_zip_reader_entry_open(zipReader);
        if (error != MZ_OK) goto cleanup_file;

        if (const auto read = mz_zip_reader_entry_read(zipReader, data.data(), data.size());
            read != data.size()) error = MZ_READ_ERROR;

        mz_zip_reader_entry_close(zipReader);
        cleanup_file:
        mz_zip_reader_close(zipReader);
        cleanup_zip_reader:
        mz_zip_reader_delete(&zipReader);

        if (error != MZ_OK) {
            // return unexpected
            return {};
        }

        auto stream = std::istringstream(data);
        return parseRegion(stream);
    }

    void Map::clearRegions() {
    }

    void Map::removeRegion(int x, int z) {
    }

    RegionImage Map::generateImage(int x, int z) const {
        return {};
    }

    std::list<RegionImage> Map::generateImages() const {
        return {};
    }

} // xaero