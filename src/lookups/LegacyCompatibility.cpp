#include "LegacyCompatibility.hpp"
#include <map>
#include <functional>
#include <tag_compound.h>
#include <tag_string.h>
#include <memory>
#include <string>

std::string_view xaero::fixBiome(const std::string_view &biome) {
    static const std::map<std::string_view, std::string_view> lookup {
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

std::string_view xaero::getBiomeFromID(const std::uint32_t biomeID) {
    static constexpr std::string_view lookup[] = {
        "ocean","plains","desert","mountains","forest","taiga","swamp","river","nether_wastes","the_end","frozen_ocean","frozen_river","snowy_tundra","snowy_mountains","mushroom_fields","mushroom_field_shore","beach","desert_hills","wooded_hills","taiga_hills","mountain_edge","jungle","jungle_hills","jungle_edge","deep_ocean","stone_shore","snowy_beach","birch_forest","birch_forest_hills","dark_forest","snowy_taiga","snowy_taiga_hills","giant_tree_taiga","giant_tree_taiga_hills","wooded_mountains","savanna","savanna_plateau","badlands","wooded_badlands_plateau","badlands_plateau","small_end_islands","end_midlands","end_highlands","end_barrens","warm_ocean","lukewarm_ocean","cold_ocean","deep_warm_ocean","deep_lukewarm_ocean","deep_cold_ocean","deep_frozen_ocean","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","the_void","","sunflower_plains","desert_lakes","gravelly_mountains","flower_forest","taiga_mountains","swamp_hills","","","","","","ice_spikes","","","","","","","","","modified_jungle","","modified_jungle_edge","","","","tall_birch_forest","tall_birch_hills","dark_forest_hills","snowy_taiga_mountains","","giant_spruce_taiga","giant_spruce_taiga_hills","modified_gravelly_mountains","shattered_savanna","shattered_savanna_plateau","eroded_badlands","modified_wooded_badlands_plateau","modified_badlands_plateau","bamboo_jungle","bamboo_jungle_hills","soul_sand_valley","crimson_forest","warped_forest","basalt_deltas","dripstone_caves","lush_caves","","meadow","grove","snowy_slopes","snowcapped_peaks","lofty_peaks","stony_peaks"
    };

    if (biomeID >= sizeof(lookup) / sizeof(std::string_view) ||
        lookup[biomeID].empty()) {

        return "plains";
    }
    return lookup[biomeID];
}

void xaero::convertNBT(std::unique_ptr<nbt::tag_compound> &nbt, const std::int16_t majorVersion) {
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
                auto& north = properties.at("north").as<nbt::tag_string>().get();
                auto& south = properties.at("south").as<nbt::tag_string>().get();
                auto& east = properties.at("east").as<nbt::tag_string>().get();
                auto& west = properties.at("west").as<nbt::tag_string>().get();

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

    if (majorVersion < 7) {
        if (const auto name = nbt->at("Name").as<nbt::tag_string>().get();
            name == "minecraft:creaking_heart") {

            if (auto& properties = nbt->at("Properties").as<nbt::tag_compound>();
                properties.size() > 0) {

                if (properties.has_key("active")) {
                    const auto& active = properties.at("active").as<nbt::tag_string>().get();
                    properties.erase("active");
                    properties.insert("creaking_heart_state", active == "true" ? "awake" : "uprooted");
                }
            }
        }
    }
}

const xaero::BlockState * xaero::getStateFromID(const std::uint32_t stateID) {
    std::uint16_t blockID = stateID & 4095;

    if (blockID >= stateIDLookupSize) {
        blockID = 0;
    }

    std::uint16_t metaID = stateID >> 12 & 1048575;

    const auto& states = stateIDLookup[blockID];

    if (states.empty()) {
        return &stateIDLookup[0].front(); // air
    }

    if (metaID >= states.size()) {
        return &states.front();
    }

    return &states[metaID];
}
