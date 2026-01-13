#ifndef XAERO_DEFAULT_LOOKUPS
#   error "this program requires XaeroDefaultLookups!"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <args.hxx>
#include <xaero/RegionTools.hpp>
#include <thread_pool/thread_pool.h>

#include <cstring>
#include <unordered_set>
#include <generator>

int main(int argc, char** argv) {
    args::ArgumentParser parser("Program for rendering Xaero's Map data");
    parser.ShortPrefix("-");
    parser.LongPrefix("--");
    args::HelpFlag help(parser, "help", "Displays this help menu", {'h', "help"});
    args::ValueFlag<std::string> files(parser, "input", "Input directory", {'i', "input"});
    args::ValueFlag<std::string> output(parser, "output", "Output directory", {'o', "output"});
    args::ValueFlag levelsOfDetail(parser, "lod", "Levels of detail", {'l', "lod"}, 16);
    args::ValueFlag cores(parser, "cores", "Number of cores to use (will clamp to maximum hardware cores)",{'c', "cores"}, std::thread::hardware_concurrency());

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (const args::Completion& e)
    {
        std::cout << e.what();
        return 0;
    }
    catch (const args::Help&)
    {
        std::cout << parser;
        return 0;
    }
    catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::filesystem::path outputRoot(output.Get());
    outputRoot /= "tiles";
    const std::filesystem::path inputRoot(*files);

    if (exists(outputRoot)) { // remove old tree
        remove_all(outputRoot);
    }

    dp::thread_pool pool{std::min(std::thread::hardware_concurrency(), *cores)};

    for (static constexpr std::array folders = {
            std::pair{"null", "DIM0"},
            std::pair{"DIM-1", "DIM-1"},
            std::pair{"DIM1", "DIM1"},
         };
         const auto& [dimension, newDimensionName] : folders) {

        if (!exists(inputRoot / dimension)) continue;

        std::filesystem::create_directories(outputRoot / newDimensionName / "0");

        std::size_t regions = 0;

        for (const auto dimensionFolder = inputRoot / dimension / "mw$default";
            const std::filesystem::path file : std::filesystem::directory_iterator(dimensionFolder)) {

            if (file.extension() != ".zip") continue;

            regions++;

            pool.enqueue_detach([&](const std::filesystem::path& region) {
                const auto image = xaero::generateImage(region);
                // cast is for msvc stl
                auto newName = region.filename().replace_extension("png").string();
                newName[newName.find_first_of('_')] = ',';
                stbi_write_png(reinterpret_cast<const char*>((outputRoot / newDimensionName / "0" / newName).c_str()), 512, 512, 4, &image, 512 * 4);
            }, file);
        }

        pool.wait_for_tasks();

        for (int lod = -1; lod > -levelsOfDetail.Get(); lod--) {
            const auto previousFolder = outputRoot / newDimensionName / std::to_string(lod + 1);
            const auto currentFolder = outputRoot / newDimensionName / std::to_string(lod);

            create_directories(currentFolder);

            struct PairHash {
                long long operator()(const std::pair<long, long>& pair) const {
                    return pair.first | (pair.second << 32);
                }
            };

            std::unordered_set<std::pair<long, long>, PairHash> processed;

            for (const std::filesystem::path toProcess : std::filesystem::directory_iterator(previousFolder)) {
                const std::string tileName = toProcess.filename().replace_extension("").string();
                const auto center = tileName.find_first_of(',');
                long x;
                std::from_chars(&tileName.front(), &tileName.at(center), x);
                long z;
                std::from_chars(&tileName.at(center + 1), &tileName.back() + 1, z);

                if (processed.contains({x, z})) {
                    continue;
                }

                x = std::floor(static_cast<double>(x) / 2);
                z = std::floor(static_cast<double>(z) / 2);

                for (long xOffset = 0; xOffset < 2; xOffset++) {
                    for (long zOffset = 0; zOffset < 2; zOffset++) {
                        processed.insert({x * 2 + xOffset, z * 2 + zOffset});
                    }
                }

                pool.enqueue_detach([&](const long centerX, const long centerZ) {

                    xaero::RegionImage newImage;
                    for (int xOffset = 0; xOffset < 2; xOffset++) {
                        for (int zOffset = 0; zOffset < 2; zOffset++) {
                            const auto tile = previousFolder / std::format("{},{}.png", centerX * 2 + xOffset, centerZ * 2 + zOffset);

                            if (!exists(tile)) continue;

                            int dump;
                            const auto image = reinterpret_cast<xaero::RegionImage*>(stbi_load(reinterpret_cast<const char*>(tile.c_str()), &dump, &dump, &dump, 4));

                            struct SmallerImage {
                                xaero::RegionImage::Pixel pixels[256][256];
                            } resized;

                            stbir_resize_uint8_linear(reinterpret_cast<unsigned char*>(image), 512, 512, 4 * 512, reinterpret_cast<unsigned char*>(&resized), 256, 256, 4 * 256, STBIR_RGBA);
                            delete image;

                            for (int i = 0; i < 256; i++) {
                                std::memcpy(newImage[i + 256 * zOffset] + 256 * xOffset, resized.pixels + i, 256 * sizeof(xaero::RegionImage::Pixel));
                            }
                        }
                    }
                    stbi_write_png(reinterpret_cast<const char*>((currentFolder / std::format("{},{}.png", centerX, centerZ)).c_str()), 512, 512, 4, &newImage, 4 * 512);
                }, x, z);
            }

            pool.wait_for_tasks();
        }
    }

    pool.wait_for_tasks();
}
