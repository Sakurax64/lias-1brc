#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <experimental/scope>
#include <fcntl.h>
#include <future>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

struct StationStats {
  std::int32_t minTenths = 9999;
  std::int32_t maxTenths = -9999;
  std::int64_t sumTenths = 0;
  std::uint32_t numRecords = 0;

  void merge(const StationStats &other) {
    this->minTenths = std::min(this->minTenths, other.minTenths);
    this->maxTenths = std::max(this->maxTenths, other.maxTenths);
    this->sumTenths += other.sumTenths;
    this->numRecords += other.numRecords;
  }
};

struct StringHash {
  using is_transparent = void;
  size_t operator()(std::string_view sv) const {
    return std::hash<std::string_view>{}(sv);
  }
  size_t operator()(const std::string &s) const {
    return std::hash<std::string>{}(s);
  }
  size_t operator()(const char *s) const {
    return std::hash<std::string_view>{}(s);
  }
};

typedef std::unordered_map<std::string, size_t, StringHash, std::equal_to<>> StringIndexMap;

struct KeyedStationsStats {
  std::vector<StationStats> stationStatsList;
  StringIndexMap stationStatsMap;

  KeyedStationsStats() {
    this->reserve(10000);
  }

  void reserve(std::size_t size) {
    stationStatsList.reserve(size);
    stationStatsMap.reserve(size);
  }

  StationStats &findOrCreateStationStats(const std::string_view stationName) {
    // heterogeneous overloads for try_emplace don't exist yet in Clang 22.1.8
    // so we can't pass the string_view directly to try_emplace
    auto iterator = stationStatsMap.find(stationName);

    if (iterator == stationStatsMap.end()) {
      const size_t new_index = stationStatsList.size();
      auto [new_iterator, inserted] = stationStatsMap.try_emplace(std::string(stationName), new_index);
      iterator = new_iterator;
      if (!inserted) {
        throw std::runtime_error("Unexpected insertion failure for key: " + std::string(stationName));
      }

      return stationStatsList.emplace_back();
    }

    const size_t existing_index = iterator->second;
    return stationStatsList[existing_index];
  }

  void merge(const KeyedStationsStats &other) {
    for (const auto &[otherName, otherIndex] : other.stationStatsMap) {
      const auto &otherStats = other.stationStatsList[otherIndex];
      auto &thisStats = this->findOrCreateStationStats(otherName);
      thisStats.merge(otherStats);
    }
  }
};

std::pair<std::string_view, std::string_view> splitLine(std::string_view line, const char delim = ';') {
  auto pos = line.find(delim);
  if (pos == std::string_view::npos) {
    return {line, ""};
  }
  return {line.substr(0, pos), line.substr(pos + 1)};
}

KeyedStationsStats parseChunk(std::string_view chunk) {
  KeyedStationsStats keyedStationStats;

  // TODO: views::split might be slower than manually doing .find() on the view
  for (auto &&line_range : chunk | std::views::split('\n')) {
    std::string_view line(line_range);
    if (line.empty()) {
      continue;
    }

    if (line.back() == '\r') {
      line.remove_suffix(1);
    }

    auto [stationName, temperatureStr] = splitLine(line);
    auto &stationStats = keyedStationStats.findOrCreateStationStats(stationName);
    float temperature = 0;
    const auto parseResult = std::from_chars(
      temperatureStr.data(),
      temperatureStr.data() + temperatureStr.size(),
      temperature
    );
    if (parseResult.ec != std::errc()) {
      continue;
    }
    auto temperatureTenths = static_cast<std::int32_t>(temperature * 10.f);
    stationStats.minTenths = std::min(stationStats.minTenths, temperatureTenths);
    stationStats.maxTenths = std::max(stationStats.maxTenths, temperatureTenths);
    stationStats.sumTenths += temperatureTenths;
    stationStats.numRecords += 1;
  }

  return keyedStationStats;
}

std::vector<std::string_view> splitViewChunks(std::string_view view, const std::size_t chunkSizeTarget = 100'000'000) {
  assert(chunkSizeTarget > 1 && "chunk_size_target must be greater than 1");

  std::vector<std::string_view> chunks;
  chunks.reserve(view.size() / chunkSizeTarget + 1);

  const std::size_t fileSize = view.size();

  for (std::size_t chunkBegin = 0; chunkBegin < fileSize;) {
    const std::size_t bytesLeft = fileSize - chunkBegin;
    const std::size_t targetSize = std::min(chunkSizeTarget, bytesLeft);

    std::size_t chunkEnd = chunkBegin + targetSize;
    if (chunkEnd < fileSize) {
      const std::size_t newline_pos = view.find('\n', chunkEnd);
      chunkEnd = (newline_pos == std::string_view::npos) ? fileSize : newline_pos;
    }

    const std::string_view rawChunk = view.substr(chunkBegin, chunkEnd - chunkBegin);
    // TODO: do we really care about Windows-style newlines?
    const std::size_t firstContent = rawChunk.find_first_not_of("\n\r");
    if (firstContent != std::string_view::npos) {
      const std::size_t lastContent = rawChunk.find_last_not_of("\n\r");
      chunks.emplace_back(rawChunk.substr(firstContent, lastContent - firstContent + 1));
    }

    chunkBegin = chunkEnd;
    if (chunkBegin < fileSize) {
      const std::size_t nextContent = view.find_first_not_of("\n\r", chunkBegin);
      if (nextContent == std::string_view::npos) {
        break;
      }
      chunkBegin = nextContent;
    }
  }

  return chunks;
}

struct TenthsFmt {
  std::int64_t value;
};

std::ostream &operator<<(std::ostream &os, const TenthsFmt &formatter) {
  const bool negative = formatter.value < 0;
  const std::uint64_t magnitude = negative ? static_cast<std::uint64_t>(-(formatter.value + 1)) + 1
                                           : static_cast<std::uint64_t>(formatter.value);
  const std::uint64_t whole = magnitude / 10;
  const std::uint64_t fraction = magnitude % 10;

  if (negative) {
    os.put('-');
  }
  os << whole;
  os.put('.');
  os.put(static_cast<char>('0' + fraction));
  return os;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <input-file>\n";
    return 1;
  }

  const char *inputPath = argv[1];

  int fd = open(inputPath, O_RDONLY);
  if (fd == -1) {
    std::cerr << "Failed to open '" << inputPath << "': " << std::strerror(errno) << "\n";
    return 1;
  }
  auto fdCleanup = std::experimental::scope_exit([&fd]() {
    if (fd != -1) {
      (void)::close(fd);
    }
  });

  struct stat fileStats{};
  if (fstat(fd, &fileStats) == -1) {
    std::cerr << "Failed to stat '" << inputPath << "': " << std::strerror(errno) << "\n";
    return 1;
  }

  const std::size_t fileSize = static_cast<std::size_t>(fileStats.st_size);
  if (fileSize == 0) {
    std::cout << "Input file is empty\n";
    return 0;
  }

  void *mapping = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
  if (mapping == MAP_FAILED) {
    std::cerr << "Failed to mmap '" << inputPath << "': " << std::strerror(errno) << "\n";
    return 1;
  }
  auto mmapCleanup = std::experimental::scope_exit([&mapping, &fileSize]() {
    if (munmap(mapping, fileSize) == -1) {
      std::cerr << "Failed to unmap file: " << std::strerror(errno) << "\n";
    }
  });

  std::string_view fullMappedView(static_cast<const char *>(mapping), fileSize);
  std::vector<std::string_view> mappedChunks = splitViewChunks(fullMappedView);

  KeyedStationsStats finalStats;
  const std::size_t maxNumTasks = std::max(1u, std::thread::hardware_concurrency());
  std::vector<std::future<std::invoke_result_t<decltype(&parseChunk), std::string_view>>> activeTasks;

  // No thread pool in C++26 stdlib, mimick one with task polling (spinning up new threads might be slow?)
  auto collectTasks = [&](const bool waitForAtLeastOne) {
    bool atLeastOneCompleted = false;

    while (true) {
      for (auto it = activeTasks.begin(); it != activeTasks.end();) {
        if (it->wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
          ++it;
          continue;
        }

        auto result = it->get();
        std::iter_swap(it, activeTasks.end() - 1);
        activeTasks.pop_back();
        finalStats.merge(result);
        atLeastOneCompleted = true;
      }

      if (!waitForAtLeastOne || atLeastOneCompleted || activeTasks.empty()) {
        return;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  };

  for (auto chunk : mappedChunks) {
    while (activeTasks.size() >= maxNumTasks) {
      collectTasks(true);
    }

    activeTasks.push_back(std::async(std::launch::async, parseChunk, chunk));
    collectTasks(false);
  }

  while (!activeTasks.empty()) {
    collectTasks(true);
  }

  const auto caseInsensitiveCompare = [](std::string_view lhs, std::string_view rhs) {
    return std::lexicographical_compare(
      lhs.begin(),
      lhs.end(),
      rhs.begin(),
      rhs.end(),
      [](const unsigned char lhsChar, const unsigned char rhsChar) {
        return std::tolower(lhsChar) < std::tolower(rhsChar);
      }
    );
  };

  std::vector<const StringIndexMap::value_type *> sortedStations;
  sortedStations.reserve(finalStats.stationStatsMap.size());
  for (const auto &stationEntry : finalStats.stationStatsMap) {
    sortedStations.push_back(&stationEntry);
  }

  std::sort(
    sortedStations.begin(),
    sortedStations.end(),
    [&caseInsensitiveCompare](const StringIndexMap::value_type *lhs, const StringIndexMap::value_type *rhs) {
      return caseInsensitiveCompare(lhs->first, rhs->first);
    }
  );

  for (const auto *stationEntry : sortedStations) {
    const auto &stationName = stationEntry->first;
    const auto &station = finalStats.stationStatsList[stationEntry->second];
    const auto averageTenths = station.numRecords > 0
      ? static_cast<std::int64_t>(std::llround(static_cast<double>(station.sumTenths) / station.numRecords))
      : 0;
    std::cout << stationName << "=" << TenthsFmt{station.minTenths} << "/" << TenthsFmt{station.maxTenths} << "/"
              << TenthsFmt{averageTenths} << "\n";
  }

  return 0;
}
