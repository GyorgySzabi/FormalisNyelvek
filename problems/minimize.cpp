#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "minimize.hpp"

namespace {

bool naturalLess(const std::string &a, const std::string &b) {
  size_t i = 0, j = 0;
  while (i < a.size() && j < b.size()) {
    if (std::isdigit(static_cast<unsigned char>(a[i])) &&
        std::isdigit(static_cast<unsigned char>(b[j]))) {
      size_t endA = i, endB = j;
      while (endA < a.size() &&
             std::isdigit(static_cast<unsigned char>(a[endA]))) {
        ++endA;
      }
      while (endB < b.size() &&
             std::isdigit(static_cast<unsigned char>(b[endB]))) {
        ++endB;
      }
      std::string numStrA = a.substr(i, endA - i);
      std::string numStrB = b.substr(j, endB - j);

      size_t nonZeroA = numStrA.find_first_not_of('0');
      size_t nonZeroB = numStrB.find_first_not_of('0');
      std::string normA =
          (nonZeroA == std::string::npos) ? "0" : numStrA.substr(nonZeroA);
      std::string normB =
          (nonZeroB == std::string::npos) ? "0" : numStrB.substr(nonZeroB);

      if (normA.size() != normB.size()) {
        return normA.size() < normB.size();
      }
      if (normA != normB) {
        return normA < normB;
      }
      if (numStrA.size() != numStrB.size()) {
        return numStrA.size() < numStrB.size();
      }
      i = endA;
      j = endB;
    } else {
      if (a[i] != b[j]) {
        return a[i] < b[j];
      }
      ++i;
      ++j;
    }
  }
  return a.size() < b.size();
}

std::string toStateName(const std::string &oldName) {
  if (!oldName.empty() && (oldName[0] == 'q' || oldName[0] == 'Q')) {
    return "s" + oldName.substr(1);
  }
  if (!oldName.empty() && oldName[0] == 's') {
    return oldName;
  }
  return "s" + oldName;
}

struct Transition {
  std::string from;
  std::string symbol;
  std::string to;

  bool operator<(const Transition &other) const {
    if (from != other.from) {
      return naturalLess(from, other.from);
    }
    if (symbol != other.symbol) {
      return naturalLess(symbol, other.symbol);
    }
    return naturalLess(to, other.to);
  }

  bool operator==(const Transition &other) const {
    return from == other.from && symbol == other.symbol && to == other.to;
  }
};

} 

void MinimizeProblem::initialize_parser(cxxopts::Options &options) {
  options.add_options()("mini", "Minimize the finite automaton");
}

bool MinimizeProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
  return args.count("mini") > 0;
}

int MinimizeProblem::run(const cxxopts::ParseResult &args) {
  if (args.count("input") == 0 || args.count("output") == 0) {
    std::cerr << "Error: Both --input and --output arguments must be provided."
              << std::endl;
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

  std::string line;

  if (!std::getline(inputFile, line)) {
    std::cerr << "Error: Missing states line in input file." << std::endl;
    return 1;
  }
  std::vector<std::string> inputStates;
  std::set<std::string> validStates;
  {
    std::istringstream iss(line);
    std::string s;
    while (iss >> s) {
      inputStates.push_back(s);
      validStates.insert(s);
    }
  }
  if (validStates.empty()) {
    std::cerr << "Error: States set cannot be empty." << std::endl;
    return 1;
  }

  
  if (!std::getline(inputFile, line)) {
    std::cerr << "Error: Missing alphabet line in input file." << std::endl;
    return 1;
  }
  std::vector<std::string> alphabet;
  std::set<std::string> validAlphabet;
  {
    std::istringstream iss(line);
    std::string sym;
    while (iss >> sym) {
      alphabet.push_back(sym);
      validAlphabet.insert(sym);
    }
  }
  if (validAlphabet.empty()) {
    std::cerr << "Error: Alphabet cannot be empty." << std::endl;
    return 1;
  }

  if (!std::getline(inputFile, line)) {
    std::cerr << "Error: Missing start state line in input file." << std::endl;
    return 1;
  }
  std::string startState;
  {
    std::istringstream iss(line);
    if (!(iss >> startState)) {
      std::cerr << "Error: Start state not specified." << std::endl;
      return 1;
    }
  }
  if (validStates.find(startState) == validStates.end()) {
    std::cerr << "Error: Start state '" << startState
              << "' is not in the list of valid states." << std::endl;
    return 1;
  }

  if (!std::getline(inputFile, line)) {
    std::cerr << "Error: Missing accept states line in input file." << std::endl;
    return 1;
  }
  std::set<std::string> acceptStates;
  {
    std::istringstream iss(line);
    std::string st;
    while (iss >> st) {
      if (validStates.find(st) == validStates.end()) {
        std::cerr << "Error: Accept state '" << st
                  << "' is not in the list of valid states." << std::endl;
        return 1;
      }
      acceptStates.insert(st);
    }
  }

  std::map<std::pair<std::string, std::string>, std::string> transitions;
  while (std::getline(inputFile, line)) {
    std::istringstream iss(line);
    std::string from, symbol, to, extra;
    if (!(iss >> from)) {
      continue;
    }
    if (!(iss >> symbol >> to)) {
      std::cerr << "Error: Malformed transition line: " << line << std::endl;
      return 1;
    }
    if (iss >> extra) {
      std::cerr << "Error: Extra tokens in transition line: " << line
                << std::endl;
      return 1;
    }

    if (validStates.find(from) == validStates.end()) {
      std::cerr << "Error: Source state '" << from << "' is not a valid state."
                << std::endl;
      return 1;
    }
    if (validAlphabet.find(symbol) == validAlphabet.end()) {
      std::cerr << "Error: Symbol '" << symbol
                << "' is not in the valid alphabet." << std::endl;
      return 1;
    }
    if (validStates.find(to) == validStates.end()) {
      std::cerr << "Error: Target state '" << to << "' is not a valid state."
                << std::endl;
      return 1;
    }

    transitions[{from, symbol}] = to;
  }
  inputFile.close();

  std::set<std::string> reachableStates;
  std::queue<std::string> q;
  q.push(startState);
  reachableStates.insert(startState);

  while (!q.empty()) {
    std::string curr = q.front();
    q.pop();

    for (const auto &sym : alphabet) {
      auto it = transitions.find({curr, sym});
      if (it != transitions.end()) {
        if (reachableStates.find(it->second) == reachableStates.end()) {
          reachableStates.insert(it->second);
          q.push(it->second);
        }
      }
    }
  }

  std::set<std::string> fGroup;
  std::set<std::string> nfGroup;
  for (const auto &st : reachableStates) {
    if (acceptStates.count(st) > 0) {
      fGroup.insert(st);
    } else {
      nfGroup.insert(st);
    }
  }

  std::vector<std::set<std::string>> partition;
  if (!nfGroup.empty()) {
    partition.push_back(nfGroup);
  }
  if (!fGroup.empty()) {
    partition.push_back(fGroup);
  }

  bool changed = true;
  while (changed) {
    changed = false;
    std::map<std::string, int> stateToBlock;
    for (size_t i = 0; i < partition.size(); ++i) {
      for (const auto &st : partition[i]) {
        stateToBlock[st] = static_cast<int>(i);
      }
    }

    std::vector<std::set<std::string>> nextPartition;
    for (const auto &block : partition) {
      if (block.size() <= 1) {
        nextPartition.push_back(block);
        continue;
      }

      std::map<std::vector<int>, std::set<std::string>> signatureMap;
      for (const auto &st : block) {
        std::vector<int> sig;
        for (const auto &sym : alphabet) {
          auto it = transitions.find({st, sym});
          if (it != transitions.end() &&
              reachableStates.count(it->second) > 0) {
            sig.push_back(stateToBlock[it->second]);
          } else {
            sig.push_back(-1);
          }
        }
        signatureMap[sig].insert(st);
      }

      if (signatureMap.size() > 1) {
        changed = true;
      }
      for (auto &pair : signatureMap) {
        nextPartition.push_back(std::move(pair.second));
      }
    }
    partition = std::move(nextPartition);
  }

  std::map<std::string, std::string> oldToNew;
  std::vector<std::string> outputStates;
  std::set<std::string> outputStatesSet;

  for (const auto &block : partition) {
    std::string rep =
        *std::min_element(block.begin(), block.end(), naturalLess);
    std::string newName = toStateName(rep);

    for (const auto &st : block) {
      oldToNew[st] = newName;
    }
    if (outputStatesSet.insert(newName).second) {
      outputStates.push_back(newName);
    }
  }

  std::sort(outputStates.begin(), outputStates.end(), naturalLess);

  std::string newStartState = oldToNew[startState];

  std::set<std::string> newAcceptSet;
  for (const auto &st : acceptStates) {
    if (oldToNew.find(st) != oldToNew.end()) {
      newAcceptSet.insert(oldToNew[st]);
    }
  }

  std::string acceptStatesLine;
  bool firstAccept = true;
  for (const auto &st : outputStates) {
    if (newAcceptSet.count(st) > 0) {
      if (!firstAccept) {
        acceptStatesLine += " ";
      }
      acceptStatesLine += st;
      firstAccept = false;
    }
  }

  std::vector<Transition> minimizedTransitions;
  std::set<Transition> uniqueTransitions;

  for (const auto &block : partition) {
    const std::string &rep = *block.begin();
    for (const auto &sym : alphabet) {
      auto it = transitions.find({rep, sym});
      if (it != transitions.end() &&
          reachableStates.count(it->second) > 0) {
        std::string from = oldToNew[rep];
        std::string to = oldToNew[it->second];
        Transition tr{from, sym, to};
        if (uniqueTransitions.insert(tr).second) {
          minimizedTransitions.push_back(tr);
        }
      }
    }
  }

  std::sort(minimizedTransitions.begin(), minimizedTransitions.end());

  std::vector<std::string> outputLines;

  std::string statesLine;
  for (size_t i = 0; i < outputStates.size(); ++i) {
    if (i > 0) {
      statesLine += " ";
    }
    statesLine += outputStates[i];
  }
  outputLines.push_back(statesLine);

  std::string alphabetLine;
  for (size_t i = 0; i < alphabet.size(); ++i) {
    if (i > 0) {
      alphabetLine += " ";
    }
    alphabetLine += alphabet[i];
  }
  outputLines.push_back(alphabetLine);

  outputLines.push_back(newStartState);
  outputLines.push_back(acceptStatesLine);

  for (const auto &tr : minimizedTransitions) {
    outputLines.push_back(tr.from + " " + tr.symbol + " " + tr.to);
  }

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
