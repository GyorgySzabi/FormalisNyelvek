#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>

#include "dfa.hpp"

// Initialize the parser for the DFA problem
void DfaProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("check", "Check word(s) against DFA (comma-separated)", cxxopts::value<std::string>());
}

// Check if the DFA problem is chosen
bool DfaProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0;
}

// Run the DFA problem
int DfaProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();
    std::string checkArg = args["check"].as<std::string>();

    // Parse comma-separated words to check
    std::vector<std::string> words;
    std::stringstream ss(checkArg);
    std::string token;
    while (std::getline(ss, token, ',')) {
        words.push_back(token);
    }

    // Read the DFA definition from the input file
    std::ifstream inputFile(inputFilename);
    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    std::string line;

    // Line 1: States (space-separated) — not strictly needed, but read it
    std::getline(inputFile, line);

    // Line 2: Alphabet (space-separated) — not strictly needed
    std::getline(inputFile, line);

    // Line 3: Start state
    std::string startState;
    std::getline(inputFile, line);
    {
        std::istringstream iss(line);
        iss >> startState;
    }

    // Line 4: Accept states (space-separated)
    std::set<std::string> acceptStates;
    std::getline(inputFile, line);
    {
        std::istringstream iss(line);
        std::string state;
        while (iss >> state) {
            acceptStates.insert(state);
        }
    }

    // Remaining lines: Transitions (format: source symbol destination)
    // Key: (current_state, symbol) -> next_state
    std::map<std::pair<std::string, std::string>, std::string> transitions;
    while (std::getline(inputFile, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string from, symbol, to;
        if (iss >> from >> symbol >> to) {
            transitions[{from, symbol}] = to;
        }
    }

    inputFile.close();

    // Simulate the DFA for each word and write results
    std::ofstream outputFile(outputFilename);
    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return 1;
    }

    for (size_t i = 0; i < words.size(); i++) {
        const std::string &word = words[i];
        std::string currentState = startState;
        bool valid = true;

        for (char c : word) {
            std::string symbol(1, c);
            auto it = transitions.find({currentState, symbol});
            if (it == transitions.end()) {
                // No transition defined for this (state, symbol) pair — reject
                valid = false;
                break;
            }
            currentState = it->second;
        }

        if (valid && acceptStates.count(currentState) > 0) {
            outputFile << "IGEN";
        } else {
            outputFile << "NEM";
        }

        if (i < words.size() - 1) {
            outputFile << "\n";
        }
    }

    outputFile.close();
    return 0;
}
