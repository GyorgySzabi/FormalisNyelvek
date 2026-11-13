#include <array>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "regex.hpp"

namespace {

const std::regex &logcatLineRegex() {
  // Captures: [1]=timestamp, [2]=PID, [3]=TID, [4]=level,
  // [5]=tag, [6]=optional message (allowing empty messages after ": ").
  static const std::regex pattern(
      R"(^([0-9-]+ [0-9:.]+) +([0-9]+) +([0-9]+) ([IDVEFAW]) ([^\r]+?):(?: ([^\r]*))?\r?$)",
      std::regex::ECMAScript | std::regex::optimize);
  return pattern;
}

const std::regex &errorLevelRegex() {
  static const std::regex pattern("E",
                                  std::regex::ECMAScript | std::regex::optimize);
  return pattern;
}

int writeOutputLines(const std::string &outputFilename,
                     const std::vector<std::string> &outputLines) {
  std::ofstream outputFile(outputFilename);
  if (!outputFile.is_open()) {
    std::cerr << "Error: Unable to open output file: " << outputFilename
              << std::endl;
    return 1;
  }

  for (size_t i = 0; i < outputLines.size(); ++i) {
    outputFile << outputLines[i];
    if (i + 1 < outputLines.size()) {
      outputFile << "\n";
    }
  }
  outputFile.close();
  return 0;
}

std::vector<std::string> solveTask1(std::ifstream &inputFile) {
  const auto flags = std::regex::ECMAScript | std::regex::optimize;
  static const std::regex reDate1015(R"(^10-15 )", flags);
  static const std::regex reExactTime(R"(^10-15 10:18:51(\.|$))", flags);
  static const std::regex reSecond51(R"(^[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:51(\.|$))",
                                     flags);
  static const std::regex reLowMemoryKiller(R"(lowmemorykiller)", flags);
  static const std::regex reTime101851(R"(^[0-9]{2}-[0-9]{2} 10:18:51(\.|$))",
                                        flags);
  static const std::regex rePid221(R"(221)", flags);
  static const std::regex reLevelW(R"(W)", flags);
  static const std::regex reTagPackageManager(R"(PackageManager\s*)", flags);
  static const std::regex reLevelD(R"(D)", flags);
  static const std::regex reTagExoPlayer(R"(ExoPlayer)", flags);
  static const std::regex reLevelEW(R"([EW])", flags);
  static const std::regex reJavaPos(R"(\.java:)", flags);
  static const std::regex reThread(R"(\bThread\b)", flags);

  const std::regex &lineRegex = logcatLineRegex();
  const std::regex &reLevelE = errorLevelRegex();

  std::array<int, 10> counts{};
  std::string line;
  std::smatch m;

  while (std::getline(inputFile, line)) {
    if (!std::regex_match(line, m, lineRegex)) {
      continue;
    }

    const auto &ts = m[1];
    const auto &pid = m[2];
    const auto &level = m[4];
    const auto &tag = m[5];
    const auto &msg = m[6];

    if (std::regex_search(ts.first, ts.second, reDate1015)) {
      ++counts[0];
    }
    if (std::regex_search(ts.first, ts.second, reExactTime)) {
      ++counts[1];
    }
    if (std::regex_search(ts.first, ts.second, reSecond51)) {
      ++counts[2];
    }
    if (std::regex_search(tag.first, tag.second, reLowMemoryKiller)) {
      ++counts[3];
    }
    if (std::regex_search(ts.first, ts.second, reTime101851) &&
        std::regex_match(pid.first, pid.second, rePid221)) {
      ++counts[4];
    }
    if (std::regex_match(level.first, level.second, reLevelE)) {
      ++counts[5];
    }
    if (std::regex_match(level.first, level.second, reLevelW) &&
        std::regex_match(tag.first, tag.second, reTagPackageManager)) {
      ++counts[6];
    }
    if (std::regex_match(level.first, level.second, reLevelD) &&
        std::regex_search(tag.first, tag.second, reTagExoPlayer)) {
      ++counts[7];
    }
    if (msg.matched &&
        std::regex_match(level.first, level.second, reLevelEW) &&
        std::regex_search(msg.first, msg.second, reJavaPos)) {
      ++counts[8];
    }
    if (msg.matched && std::regex_search(msg.first, msg.second, reThread)) {
      ++counts[9];
    }
  }

  std::vector<std::string> result;
  result.reserve(counts.size());
  for (int c : counts) {
    result.push_back(std::to_string(c));
  }
  return result;
}

std::vector<std::string> solveTask2(std::ifstream &inputFile) {
  static const std::regex reStackAt(R"(^\s*at\s+)",
                                    std::regex::ECMAScript | std::regex::optimize);
  static const std::regex reJavaFile(R"(\(([^():\s]+\.java):[0-9]+\))",
                                     std::regex::ECMAScript | std::regex::optimize);
  const std::regex &lineRegex = logcatLineRegex();
  const std::regex &reLevelE = errorLevelRegex();

  std::vector<std::string> files;
  std::string line;
  std::smatch m;

  while (std::getline(inputFile, line)) {
    if (!std::regex_match(line, m, lineRegex)) {
      continue;
    }
    if (!std::regex_match(m[4].first, m[4].second, reLevelE) || !m[6].matched) {
      continue;
    }
    if (!std::regex_search(m[6].first, m[6].second, reStackAt)) {
      continue;
    }

    for (std::sregex_iterator it(m[6].first, m[6].second, reJavaFile), end;
         it != end; ++it) {
      files.push_back((*it)[1].str());
    }
  }

  return files;
}

std::vector<std::string> solveTask3(std::ifstream &inputFile) {
  const std::regex &lineRegex = logcatLineRegex();
  std::unordered_set<std::string> seenPids;
  std::vector<std::string> orderedPids;
  std::string line;
  std::smatch m;

  while (std::getline(inputFile, line)) {
    if (!std::regex_match(line, m, lineRegex)) {
      continue;
    }
    std::string pid = m[2].str();
    if (seenPids.insert(pid).second) {
      orderedPids.push_back(std::move(pid));
    }
  }

  return orderedPids;
}

std::vector<std::string> solveTask4(std::ifstream &inputFile) {
  const std::regex &lineRegex = logcatLineRegex();
  const std::regex &reLevelE = errorLevelRegex();

  std::unordered_map<std::string, int> errorCounts;
  std::vector<std::string> orderedPids;
  std::string line;
  std::smatch m;

  while (std::getline(inputFile, line)) {
    if (!std::regex_match(line, m, lineRegex)) {
      continue;
    }
    if (!std::regex_match(m[4].first, m[4].second, reLevelE)) {
      continue;
    }
    std::string pid = m[2].str();
    int &count = errorCounts[pid];
    if (count == 0) {
      orderedPids.push_back(pid);
    }
    ++count;
  }

  if (orderedPids.empty()) {
    return {};
  }

  std::string maxPid = orderedPids[0];
  int maxErrors = errorCounts[maxPid];
  for (const std::string &pid : orderedPids) {
    int count = errorCounts[pid];
    if (count > maxErrors) {
      maxErrors = count;
      maxPid = pid;
    }
  }

  return {"PID=" + maxPid + " (" + std::to_string(maxErrors) + " errors)"};
}

std::vector<std::string> solveTask5(std::ifstream &inputFile) {
  const std::regex &lineRegex = logcatLineRegex();
  const std::regex &reLevelE = errorLevelRegex();

  int mainThreadErrors = 0;
  std::string line;
  std::smatch m;

  while (std::getline(inputFile, line)) {
    if (!std::regex_match(line, m, lineRegex)) {
      continue;
    }
    if (std::regex_match(m[4].first, m[4].second, reLevelE) && m[2] == m[3]) {
      ++mainThreadErrors;
    }
  }

  return {std::to_string(mainThreadErrors)};
}

} // namespace

void RegexProblem::initialize_parser(cxxopts::Options &options) {
  options.add_options()("rgx", "Run regex search task (1-5)",
                        cxxopts::value<int>());
}

bool RegexProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
  return args.count("rgx") > 0;
}

int RegexProblem::run(const cxxopts::ParseResult &args) {
  if (args.count("input") == 0 || args.count("output") == 0) {
    std::cerr << "Error: Both --input and --output arguments must be provided."
              << std::endl;
    return 1;
  }

  int taskId = args["rgx"].as<int>();
  if (taskId < 1 || taskId > 5) {
    std::cerr << "Error: --rgx task number must be between 1 and 5, got: "
              << taskId << std::endl;
    return 1;
  }

  std::string inputFilename = args["input"].as<std::string>();
  std::string outputFilename = args["output"].as<std::string>();

  std::ifstream inputFile(inputFilename);
  if (!inputFile.is_open()) {
    std::cerr << "Error: Unable to open input file: " << inputFilename
              << std::endl;
    return 1;
  }

  std::vector<std::string> outputLines;
  switch (taskId) {
  case 1:
    outputLines = solveTask1(inputFile);
    break;
  case 2:
    outputLines = solveTask2(inputFile);
    break;
  case 3:
    outputLines = solveTask3(inputFile);
    break;
  case 4:
    outputLines = solveTask4(inputFile);
    break;
  case 5:
    outputLines = solveTask5(inputFile);
    break;
  }
  inputFile.close();

  return writeOutputLines(outputFilename, outputLines);
}
