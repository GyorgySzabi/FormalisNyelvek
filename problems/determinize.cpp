#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>

#include "determinize.hpp"

namespace {
struct DfaTransition {
    int fromId;
    std::string symbol;
    int toId;

    bool operator<(const DfaTransition& other) const {
        if (fromId != other.fromId) {
            return fromId < other.fromId;
        }
        if (symbol != other.symbol) {
            return symbol < other.symbol;
        }
        return toId < other.toId;
    }
};
} // namespace

void DeterminizeProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("det", "Determinize the finite automaton");
}

bool DeterminizeProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("det") > 0;
}

int DeterminizeProblem::run(const cxxopts::ParseResult &args) {
    if (args.count("input") == 0 || args.count("output") == 0) {
        std::cerr << "Error: Both --input and --output arguments must be provided." << std::endl;
        return 1;
    }

    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Unable to open input file: " << inputFilename << std::endl;
        return 1;
    }

    std::string line;

    // Line 1: States
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

    // Line 2: Alphabet
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

    // Line 3: Start state
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
        std::cerr << "Error: Start state '" << startState << "' is not in the list of valid states." << std::endl;
        return 1;
    }

    // Line 4: Final states
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
                std::cerr << "Error: Accept state '" << st << "' is not in the list of valid states." << std::endl;
                return 1;
            }
            acceptStates.insert(st);
        }
    }

    // Remaining lines: Transitions
    std::map<std::pair<std::string, std::string>, std::set<std::string>> nfaTransitions;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line);
        std::string from, symbol, to, extra;
        if (!(iss >> from)) {
            continue; // Empty line
        }
        if (!(iss >> symbol >> to)) {
            std::cerr << "Error: Malformed transition line: " << line << std::endl;
            return 1;
        }
        if (iss >> extra) {
            std::cerr << "Error: Extra tokens in transition line: " << line << std::endl;
            return 1;
        }

        // Validation against valid states and alphabet
        if (validStates.find(from) == validStates.end()) {
            std::cerr << "Error: Source state '" << from << "' is not a valid state." << std::endl;
            return 1;
        }
        if (validAlphabet.find(symbol) == validAlphabet.end()) {
            std::cerr << "Error: Symbol '" << symbol << "' is not in the valid alphabet." << std::endl;
            return 1;
        }
        if (validStates.find(to) == validStates.end()) {
            std::cerr << "Error: Target state '" << to << "' is not a valid state." << std::endl;
            return 1;
        }

        nfaTransitions[{from, symbol}].insert(to);
    }
    inputFile.close();

    // Determinization via Powerset / Subset Construction (BFS)
    std::vector<std::set<std::string>> dfaStates;
    std::map<std::set<std::string>, int> stateMap;
    std::queue<int> queue;

    std::set<std::string> initialDfaState = {startState};
    dfaStates.push_back(initialDfaState);
    stateMap[initialDfaState] = 0;
    queue.push(0);

    std::vector<DfaTransition> dfaTransitions;

    while (!queue.empty()) {
        int currentId = queue.front();
        queue.pop();

        std::set<std::string> currentSet = dfaStates[currentId];

        // Process alphabet symbols in the order given in the input alphabet
        for (const auto &symbol : alphabet) {
            std::set<std::string> targetSet;

            for (const auto &nfaState : currentSet) {
                auto it = nfaTransitions.find({nfaState, symbol});
                if (it != nfaTransitions.end()) {
                    for (const auto &dest : it->second) {
                        targetSet.insert(dest);
                    }
                }
            }

            // If there are no outgoing transitions, no edge is created
            if (targetSet.empty()) {
                continue;
            }

            int targetId;
            auto it = stateMap.find(targetSet);
            if (it == stateMap.end()) {
                targetId = static_cast<int>(dfaStates.size());
                dfaStates.push_back(targetSet);
                stateMap[targetSet] = targetId;
                queue.push(targetId);
            } else {
                targetId = it->second;
            }

            dfaTransitions.push_back({currentId, symbol, targetId});
        }
    }

    // Sort transitions: fromId -> symbol -> toId
    std::sort(dfaTransitions.begin(), dfaTransitions.end());

    // Prepare output lines
    std::vector<std::string> outputLines;

    // Line 1: States (s0 s1 s2 ...)
    std::string statesLine;
    for (size_t i = 0; i < dfaStates.size(); ++i) {
        if (i > 0) statesLine += " ";
        statesLine += "s" + std::to_string(i);
    }
    outputLines.push_back(statesLine);

    // Line 2: Alphabet
    std::string alphabetLine;
    for (size_t i = 0; i < alphabet.size(); ++i) {
        if (i > 0) alphabetLine += " ";
        alphabetLine += alphabet[i];
    }
    outputLines.push_back(alphabetLine);

    // Line 3: Start state (always s0)
    outputLines.push_back("s0");

    // Line 4: Final states
    std::string acceptStatesLine;
    bool firstAccept = true;
    for (size_t i = 0; i < dfaStates.size(); ++i) {
        bool isAccept = false;
        for (const auto &st : dfaStates[i]) {
            if (acceptStates.count(st) > 0) {
                isAccept = true;
                break;
            }
        }
        if (isAccept) {
            if (!firstAccept) acceptStatesLine += " ";
            acceptStatesLine += "s" + std::to_string(i);
            firstAccept = false;
        }
    }
    outputLines.push_back(acceptStatesLine);

    // Transitions lines
    for (const auto &tr : dfaTransitions) {
        outputLines.push_back("s" + std::to_string(tr.fromId) + " " + tr.symbol + " s" + std::to_string(tr.toId));
    }

    // Write to output file
    std::ofstream outputFile(outputFilename);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Unable to open output file: " << outputFilename << std::endl;
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
