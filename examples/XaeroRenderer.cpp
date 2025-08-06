#ifndef XAERO_DEFAULT_LOOKUPS
#   error "this program requires XaeroDefaultLookups!"
#endif

#include <args.hxx>
#include <xaero/Map.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

xaero::RegionImage getImage (const std::filesystem::path& data) {
    std::cout << "Getting: " << data.string() << "\n";

    return xaero::Map::generateImage(xaero::Map::parseRegion(data), &xaero::defaultLookupPack);
}

void writeImage(const xaero::RegionImage& image, const std::filesystem::path& path) {
    stbi_write_png(reinterpret_cast<const char*>(path.c_str()), 512, 512, 4, &image, 512 * 4);
}

int main(int argc, char** argv) {
    args::ArgumentParser parser("Program for rendering Xaero's Map data");
    parser.ShortPrefix("-");
    parser.LongPrefix("--");
    args::HelpFlag help(parser, "help", "Displays this help menu", {'h', "help"});
    args::ValueFlag<std::string> files(parser, "input", "Input file or directory", {'i', "input"});
    args::ValueFlag<std::string> output(parser, "output", "Output directory", {'o', "output"});

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

    const std::filesystem::path outputRoot(*output);

    if (const std::filesystem::path input(*files);
        is_directory(input)) {
        for (const auto& file : std::filesystem::directory_iterator(input)) {
            const auto& inputPath = file.path();
            if (file.is_directory() || inputPath.extension() != ".zip") continue;

            writeImage(getImage(inputPath), outputRoot / inputPath.filename().replace_extension("png") );
        }
    } else {
        writeImage(getImage(input), outputRoot / input.filename().replace_extension("png") );
    }
}
