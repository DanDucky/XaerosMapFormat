#include "../include/xaero/Map.hpp"

#include <fstream>
#include <functional>
#include <memory>
#include <algorithm>
#include <spanstream>
#include <format>

#include "xaero/types/RegionImage.hpp"
#include "xaero/types/Region.hpp"
#include "xaero/types/LookupTypes.hpp"
#include "lookups/LegacyCompatibility.hpp"
#include "util/ByteInputStream.hpp"
#include "util/ByteOutputStream.hpp"

#include <nbt_tags.h>

// for some reason when I remove this include I get errors, so keep mz_strm
#include <mz_strm.h>
#include <mz.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>
#include <mz_strm_mem.h>
#include <mz_strm_os.h>

#include "util/StringUtils.hpp"

namespace xaero {

    // so these are entirely internal to this library, that's why they're not exposed
    // mojang doesn't use these same indices and when I read in the blocks I manually set these unless mojang "got them right"
    enum class TintIndex : std::int32_t {
        NONE = -1,
        GRASS = 0,
        FOLIAGE = 1,
        DRY_FOLIAGE = 2,
        REDSTONE = 3,
        WATER = 4
    };

    enum class ColorType : std::uint8_t { // legacy color type system, not supported by Xaero and not rendered by this
        NONE = 0,
        DEFAULT = 1,
        CUSTOM_COLOR = 2,
        CUSTOM_BIOME = 3
    }; // the names here are GUESSES and probably incorrect, thank Xaero for using magic numbers everywhere and then not providing source code with comments

    std::string_view getBiome(const std::variant<std::shared_ptr<std::string>, std::string, std::string_view>& biome) {
        std::string_view out;
        if (std::holds_alternative<std::shared_ptr<std::string>>(biome)) {
            out = *std::get<std::shared_ptr<std::string>>(biome);
        } else if (std::holds_alternative<std::string>(biome)) {
            out = std::get<std::string>(biome);
        } else if (std::holds_alternative<std::string_view>(biome)) {
            out = std::get<std::string_view>(biome);
        }

        return stripName(out);
    }

    const BlockState* getState(const std::variant<std::monostate, BlockState, std::shared_ptr<BlockState>, const BlockState*>& state) {
        if (std::holds_alternative<std::monostate>(state)) {
            return getStateFromID(0); // air
        }
        if (std::holds_alternative<const BlockState*>(state)) {
            return std::get<const BlockState*>(state);
        }
        if (std::holds_alternative<BlockState>(state)) {
            return &std::get<BlockState>(state);
        }
        return std::get<std::shared_ptr<BlockState>>(state).get();
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

            if (region.majorVersion > 7 || region.minorVersion > 8) {
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
                            parameters.skipBits(1); // for some reason this is ignored????
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
                                    pixel.state = getStateFromID(state);
                                } else {
                                    if (newStatePaletteEntry) {
                                        auto nbtStream = nbt::io::stream_reader(stream.getStream());

                                        auto nbt = nbtStream.read_compound();

                                        if (region.majorVersion < 7) {
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
                                pixel.state = getStateFromID(2); // grass block
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
                                        overlay.state = getStateFromID(9); // default water state
                                    } else {
                                        if (region.majorVersion == 0) {
                                            overlay.state = getStateFromID(stream.getNext<std::int32_t>());
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
                                                fixBiome(stripName(biomeName)) :
                                                std::move(stripName(biomeName));
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
                                stream.skip(1);
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

    inline void serializeRegionImpl(const Region& region, ByteOutputStream& stream, const LookupPack* lookups) {
        stream.write<std::uint8_t>(255); // has version

        stream.write<std::uint16_t>(7); // "major version"
        stream.write<std::uint16_t>(8); // "minor version"

        std::vector<const BlockState*> statePalette;
        auto findStateInPalette = [&statePalette](const BlockState* state) -> std::optional<std::size_t> {
            const auto found = std::ranges::find_if(statePalette.begin(), statePalette.end(), [&state](const BlockState* b) -> bool {
                return *state == *b;
            });

            if (found == statePalette.end()) {
                return std::nullopt;
            }
            return found - statePalette.begin();
        };
        std::vector<std::string_view> biomePalette;

        for (std::uint8_t tileX = 0; tileX < 8; tileX++) {
            for (std::uint8_t tileZ = 0; tileZ < 8; tileZ++) {
                if (!region[tileX][tileZ].isPopulated()) continue;

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

                                auto state = getState(pixel.state);
                                const bool isGrass = state->isName("grass_block");

                                parameters.writeNext(!isGrass, 1);
                                parameters.writeNext(pixel.hasOverlays(), 1);
                                parameters.writeNext(ColorType::NONE, 2);
                                parameters.skip(2); // slope is no longer supported and next isn't used
                                parameters.writeNext(false, 1);
                                parameters.skipToNextByte();
                                parameters.writeNext(pixel.light, 4);
                                parameters.writeNext(pixel.height, 8);
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
                                    biome = getBiome(pixel.biome.value());

                                    if (const auto found = std::ranges::find(biomePalette.begin(), biomePalette.end(), biome);
                                        found == biomePalette.end()) {

                                        biomeInPalette = false;
                                    } else {
                                        biomeInPalette = true;
                                        biomePaletteIndex = found - biomePalette.begin();
                                    }
                                }

                                parameters.writeNext(!biomeInPalette, 1);
                                parameters.writeNext(false, 1); // biome is not id
                                parameters.writeNext(pixel.topHeight.has_value(), 1);
                                parameters.writeNext(pixel.height >> 8, 4);

                                stream.write(parameters); // pixel parameters

                                if (!isGrass) {
                                    if (stateInPalette) {
                                        stream.write<std::uint32_t>(statePaletteIndex);
                                    } else {
                                        nbt::io::stream_writer nbtWriter(stream.getStream());

                                        nbtWriter.write_tag("", state->getNBT()); // for some reason needs an empty key...

                                        statePalette.push_back(std::move(state));
                                    }
                                }

                                if (pixel.topHeight.has_value()) {
                                    stream.write<std::uint8_t>(pixel.topHeight.value());
                                }

                                if (pixel.hasOverlays()) {
                                    stream.write<std::uint8_t>(pixel.overlays.size());

                                    for (const auto& overlay : pixel.overlays) {
                                        BitWriter<std::uint32_t> overlayParameters;

                                        const auto overlayState = getState(overlay.state);
                                        const bool isWater = overlayState->isName("water");

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

                                                nbtWriter.write_payload(state->getNBT());

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


    std::string Map::serializeRegion(const Region &region, const LookupPack *lookups) {
        auto stringStream = std::ostringstream();
        ByteOutputStream stream(stringStream);
        serializeRegionImpl(region, stream, lookups);
        return stringStream.str();
    }

    inline std::int32_t packRegionImpl(void* const stream, const std::string_view &serialized) {
        std::int32_t error = MZ_OK;
        void* zipHandle = nullptr;
        mz_zip_file fileInfo = {};

        zipHandle = mz_zip_create();
        if (!zipHandle) {
            error = MZ_STREAM_ERROR; // I know this isn't entirely right but whatever
            goto cleanup;
        }

        error = mz_zip_open(zipHandle, stream, MZ_OPEN_MODE_WRITE);
        if (error != MZ_OK) goto cleanup;

        fileInfo.filename = "region.xaero";
        fileInfo.uncompressed_size = serialized.size();
        fileInfo.compression_method = MZ_COMPRESS_METHOD_DEFLATE;

        error = mz_zip_entry_write_open(zipHandle, &fileInfo, MZ_COMPRESS_LEVEL_NORMAL, false, nullptr);
        if (error != MZ_OK) goto cleanup;

        error = mz_zip_entry_write(zipHandle, serialized.data(), serialized.size());

        cleanup:
        if (zipHandle == nullptr) return error;
        mz_zip_entry_close(zipHandle);
        mz_zip_close(zipHandle);
        mz_zip_delete(&zipHandle);
        return error;
    }

    std::string Map::packRegion(const std::string_view &serialized) {
        std::int32_t error = MZ_OK;
        std::int32_t size = 0;
        std::string output;
        const void* buffer = nullptr;
        void* streamHandle = nullptr;

        streamHandle = mz_stream_mem_create();
        if (!streamHandle) {
            error = MZ_STREAM_ERROR;
            goto cleanup;
        }

        error = mz_stream_mem_open(streamHandle, nullptr, MZ_OPEN_MODE_CREATE);
        if (error != MZ_OK) goto cleanup;


        error = packRegionImpl(streamHandle, serialized);
        if (error != MZ_OK) goto cleanup;

        mz_stream_mem_get_buffer_length(streamHandle, &size);
        error = mz_stream_mem_get_buffer(streamHandle, &buffer);
        if (error != MZ_OK) goto cleanup;
        output.resize(size);
        std::memcpy(output.data(), buffer, size);

        cleanup :
        if (streamHandle) {
            mz_stream_mem_close(streamHandle);
            mz_stream_mem_delete(&streamHandle);
        }

        if (error != MZ_OK) return "";

        return output;
    }

    bool Map::writeRegion(const std::string_view &serialized, const std::filesystem::path &path) {
        std::int32_t error = MZ_OK;
        void* streamHandle = nullptr;

        streamHandle = mz_stream_os_create();
        if (!streamHandle) goto cleanup;

        error = mz_stream_os_open(streamHandle, reinterpret_cast<const char*>(path.c_str()), MZ_OPEN_MODE_CREATE | MZ_OPEN_MODE_WRITE | MZ_OPEN_MODE_EXISTING);
        if (error != MZ_OK) goto cleanup;

        error = packRegionImpl(streamHandle, serialized);

        cleanup:
        if (streamHandle) {
            mz_stream_os_close(streamHandle);
            mz_stream_os_delete(&streamHandle);
        }

        return error == MZ_OK;
    }

    float getBrightnessMultiplier (const std::uint8_t min, const std::uint8_t light, const std::uint8_t sun=15) {
        return static_cast<float>(min) + static_cast<float>(std::max(sun, light)) / (15.0F + min);
    }

    // separated so it can be used on both the overlays and the pixels
    RegionImage::Pixel getStateColor(
        const std::variant<std::monostate, BlockState, std::shared_ptr<BlockState>, const BlockState*>& stateVariant,
        const BiomeColors& biome,
        const LookupPack* lookups) {

        RegionImage::Pixel color;
        TintIndex tint;
        const auto state = getState(stateVariant);

        if (const auto properties = lookups->stateLookup.find(state->strippedName());
            properties != lookups->stateLookup.end()) {

            const auto& pack = properties->second.at(state->properties);
            color = pack.color;
            tint = static_cast<TintIndex>(pack.tintIndex);
        } else {
            tint = TintIndex::NONE;
        }

        if (tint != TintIndex::NONE) {
            if (const auto strippedName = state->strippedName();
                strippedName.contains("redstone")) {
                tint = TintIndex::REDSTONE;
            } else if (strippedName == "leaf_litter") {
                tint = TintIndex::DRY_FOLIAGE;
            } else if (strippedName.contains("leaves") || strippedName == "vine") {
                tint = TintIndex::FOLIAGE;
            } else if (strippedName.contains("water")) {
                tint = TintIndex::WATER;
            }
        }

        RegionImage::Pixel tintColor = {255, 255, 255}; // will cancel out in the math;
        switch (tint) {
            case TintIndex::NONE:
                break;
            case TintIndex::GRASS:
                tintColor = biome.grass;
                break;
            case TintIndex::FOLIAGE:
                tintColor = biome.foliage;
                break;
            case TintIndex::DRY_FOLIAGE:
                tintColor = biome.dryFoliage;
                break;
            case TintIndex::REDSTONE:
                tintColor = {231, 6, 0}; // pretty random
                break;
            case TintIndex::WATER:
                tintColor = biome.water;
                break;
        }

        // apply tint
        color.red   = (color.red   * tintColor.red)   / 255;
        color.blue  = (color.blue  * tintColor.blue)  / 255;
        color.green = (color.green * tintColor.green) / 255;

        // this is where I would apply glowing effects, but I can't find a consistent way of grabbing that info from the game (without launching it)... honestly should write my own data generator mod
        // but that would only complicate the configure step for very little payoff... maybe some day if I care

        return color;
    }

    RegionImage Map::generateImage(const Region &region, const LookupPack *lookups) {
        RegionImage output;

        if (lookups == nullptr) { // bad!!!
            return output;
        }
        for (std::uint16_t tileChunkX = 0; tileChunkX < 8; tileChunkX++) {
            for (std::uint16_t tileChunkZ = 0; tileChunkZ < 8; tileChunkZ++) {
                if (!region[tileChunkX][tileChunkZ].isPopulated()) continue;
                for (std::uint16_t chunkX = 0; chunkX < 4; chunkX++) {
                    for (std::uint16_t chunkZ = 0; chunkZ < 4; chunkZ++) {
                        const auto& chunk = region[tileChunkX][tileChunkZ][chunkX][chunkZ];
                        if (!chunk.isPopulated()) continue;
                        for (std::uint8_t pixelX = 0; pixelX < 16; pixelX++) {
                            for (std::uint8_t pixelZ = 0; pixelZ < 16; pixelZ++) {
                                const std::uint16_t x = pixelX | chunkX << 4 | tileChunkZ << 6;
                                const std::uint16_t z = pixelZ | chunkZ << 4 | tileChunkX << 6;
                                const auto& pixel = chunk[pixelX][pixelZ];

                                const auto biome = pixel.biome ? getBiome(pixel.biome.value()) : "plains";

                                const auto foundBiome = lookups->biomeLookup.find(biome);
                                BiomeColors biomeColors;
                                if (foundBiome != lookups->biomeLookup.end()) {
                                    biomeColors = foundBiome->second;
                                } else {
                                    biomeColors = lookups->biomeLookup.at("plains");
                                }

                                auto color = getStateColor(pixel.state, biomeColors, lookups);

                                output[x][z] = color;

                                if (pixel.hasOverlays()) {
                                    for (const auto& overlay : pixel.overlays) {
                                        auto overlayColor = getStateColor(overlay.state, biomeColors, lookups);

                                        const auto intensity = getBrightnessMultiplier(9, overlay.light, 15);
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

    Region Map::parseRegion(const std::span<char> &data) {
        std::ispanstream stream(data);
        return parseRegion(stream);
    }

} // xaero