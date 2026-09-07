#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

constexpr uint32_t DEFAULT_SEED = 8192; // DON'T CHANGE FOR CHALLENGE
constexpr uint64_t DEFAULT_TOTAL_ROWS = 1'000'000'000;
constexpr const char* DEFAULT_OUTPUT = "measurements.txt";

int main(int argc, char* argv[]) {
    uint32_t seed = DEFAULT_SEED;
    uint64_t totalRows = DEFAULT_TOTAL_ROWS;
    std::string outputFilename = DEFAULT_OUTPUT;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-s") {
            if (++i >= argc) {
                std::cerr << "Error: -s requires a seed\n";
                return 1;
            }

            try {
                seed = std::stoul(argv[i]);
            } catch (...) {
                std::cerr << "Error: invalid seed: " << argv[i] << '\n';
                return 1;
            }
        }
        else if (arg == "-r") {
            if (++i >= argc) {
                std::cerr << "Error: -r requires a row count\n";
                return 1;
            }

            try {
                totalRows = std::stoull(argv[i]);
            } catch (...) {
                std::cerr << "Error: invalid row count: " << argv[i] << '\n';
                return 1;
            }
        }
        else if (arg == "-o") {
            if (++i >= argc) {
                std::cerr << "Error: -o requires an output filename\n";
                return 1;
            }

            outputFilename = argv[i];
        }
        else if (arg == "-h" || arg == "--help") {
            std::cout
                << "Usage: generator [options]\n"
                << "\n"
                << "Options:\n"
                << "  -s <seed>      Random seed (default: 8192)\n"
                << "  -r <rows>      Number of rows (default: 1000000000)\n"
                << "  -o <filename>  Output filename (default: measurements.txt)\n"
                << "  -h, --help     Show this help\n";

            return 0;
        }
        else {
            std::cerr << "Error: unknown argument: " << arg << '\n';
            return 1;
        }
    }

    // Warn when using a different seed
    if (seed != DEFAULT_SEED) {
        std::cout
            << "WARNING: You are using seed " << seed << " instead of "
            << DEFAULT_SEED << ".\n"
            << "This will produce different output from the dataset used "
            << "for scoring in the challenge.\n"
            << "\n"
            << "Continue? [y/N]: ";

        std::string confirmation;
        std::getline(std::cin, confirmation);

        if (confirmation != "y" && confirmation != "Y") {
            std::cout << "Cancelled.\n";
            return 0;
        }
    }

    std::ifstream stationsFile("stations.txt");

    if (!stationsFile) {
        std::cerr << "Error: could not open stations.txt\n";
        return 1;
    }

    std::vector<std::string> stations;
    std::string station;

    while (std::getline(stationsFile, station)) {
        if (!station.empty()) {
            stations.push_back(station);
        }
    }

    if (stations.empty()) {
        std::cerr << "Error: stations.txt contains no stations\n";
        return 1;
    }

    std::cout << "Loaded " << stations.size() << " stations\n";
    std::cout << "Seed: " << seed << '\n';
    std::cout << "Rows: " << totalRows << '\n';
    std::cout << "Output: " << outputFilename << '\n';

    std::ofstream output(outputFilename, std::ios::binary);

    if (!output) {
        std::cerr << "Error: could not create " << outputFilename << '\n';
        return 1;
    }

    std::mt19937 rng(seed);

    std::uniform_int_distribution<size_t> stationDist(
        0, stations.size() - 1
    );

    std::uniform_int_distribution<int> temperatureDist(-999, 999);

    std::string buffer;
    buffer.reserve(1024 * 1024);

    for (uint64_t i = 0; i < totalRows; ++i) {
        const std::string& station = stations[stationDist(rng)];
        const int temperature = temperatureDist(rng);

        buffer += station;
        buffer += ';';

        if (temperature < 0)
            buffer += '-';

        const int absolute = temperature < 0
            ? -temperature
            : temperature;

        buffer += std::to_string(absolute / 10);
        buffer += '.';
        buffer += char('0' + absolute % 10);
        buffer += '\n';

        if (buffer.size() >= 1024 * 1024) {
            output.write(buffer.data(), buffer.size());
            buffer.clear();

            if (!output) {
                std::cerr << "Error: failed while writing output\n";
                return 1;
            }
        }
    }

    if (!buffer.empty())
        output.write(buffer.data(), buffer.size());

    output.close();

    std::cout << "Generated " << outputFilename << '\n';

    return 0;
}