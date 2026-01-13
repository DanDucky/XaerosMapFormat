#include <args.hxx>
#include <filesystem>
#include <xaero/RegionTools.hpp>
#include <xaero/types/Region.hpp>

int main(const int argc, char** argv) {
    args::ArgumentParser parser("Program for reprocessing Xaero's Map data");
    parser.ShortPrefix("-");
    parser.LongPrefix("--");
    args::HelpFlag help(parser, "help", "Displays this help menu", {'h', "help"});
    args::ValueFlag<std::string> files(parser, "input", "Input file", {'i', "input"});
    args::ValueFlag<std::string> output(parser, "output", "Output directory", {'o', "output"});

    try {
        parser.ParseCLI(argc, argv);
    }
    catch (const args::Completion& e) {
        std::cout << e.what();
        return 0;
    }
    catch (const args::Help&) {
        std::cout << parser;
        return 0;
    }
    catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    const std::filesystem::path outputRoot(*output);

    const std::filesystem::path input{files.Get()};

    const auto region = xaero::parseRegion(input);

    xaero::writeRegion(region, (outputRoot / (input.stem().string() + "_reprocessed")).replace_extension(".zip"), nullptr);
}
