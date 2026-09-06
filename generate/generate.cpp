#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

constexpr uint32_t SEED = 8192; // DON'T CHANGE
constexpr uint64_t TOTAL_ROWS = 1'000'000'000;

int main() {
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

    std::ofstream output("measurements.txt", std::ios::binary);

    if (!output) {
        std::cerr << "Error: could not create measurements.txt\n";
        return 1;
    }

    std::mt19937 rng(SEED);

    std::uniform_int_distribution<size_t> stationDist(
        0, stations.size() - 1
    );

    std::uniform_int_distribution<int> temperatureDist(-999, 999);

    std::string buffer;
    buffer.reserve(1024 * 1024);

    for (uint64_t i = 0; i < TOTAL_ROWS; ++i) {
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

    std::cout << "Generated measurements.txt\n";

    return 0;
}