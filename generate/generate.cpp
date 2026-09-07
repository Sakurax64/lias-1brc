#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <cmath>

constexpr uint32_t DEFAULT_SEED = 8192; // DON'T CHANGE FOR CHALLENGE
constexpr uint64_t DEFAULT_TOTAL_ROWS = 1'000'000'000;
constexpr const char* DEFAULT_OUTPUT = "measurements.txt";

int main(int argc, char* argv[]) {
    uint32_t seed = DEFAULT_SEED;
    uint64_t totalRows = DEFAULT_TOTAL_ROWS;
    std::string outputFilename = DEFAULT_OUTPUT;

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
        if (!station.empty()) stations.push_back(station);
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

    std::uniform_real_distribution<double> meanDist(-30.0, 45.0);
    std::uniform_real_distribution<double> stdDevDist(3.0, 12.0);

    std::vector<double> stationMean(stations.size());
    std::vector<double> stationStdDev(stations.size());

    for (size_t i = 0; i < stations.size(); ++i) {
        stationMean[i]   = meanDist(rng);
        stationStdDev[i] = stdDevDist(rng);
    }

    std::uniform_int_distribution<size_t> stationDist(0, stations.size() - 1);
    std::normal_distribution<double> tempDist(0.0, 1.0);

    std::string buffer;
    buffer.reserve(1024 * 1024);

    for (uint64_t i = 0; i < totalRows; ++i) {
        const size_t idx = stationDist(rng);
        const std::string& stationName = stations[idx];

        double z = tempDist(rng);
        double raw = stationMean[idx] + z * stationStdDev[idx];
        raw = std::max(-99.9, std::min(99.9, raw));

        long long tenths = std::llround(raw * 10.0);
        int temperature = static_cast<int>(tenths);

        buffer += stationName;
        buffer += ';';

        if (temperature < 0) buffer += '-';
        const int absolute = temperature < 0 ? -temperature : temperature;

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

    if (!buffer.empty()) output.write(buffer.data(), buffer.size());
    output.close();

    std::cout << "Generated " << outputFilename << '\n';
    return 0;
}