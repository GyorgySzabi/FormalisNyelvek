#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>

#include "dfa.hpp"

void DfaProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("check", "Check word(s) against DFA (comma-separated)", cxxopts::value<std::string>());
}

bool DfaProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0;
}

int DfaProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();
    std::string checkArg = args["check"].as<std::string>();
    std::vector<std::string> words;
    std::stringstream ss(checkArg);
    std::string token;
    while (std::getline(ss, token, ',')) {
        words.push_back(token);
    }

    std::ifstream inputFile(inputFilename);
    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    std::string line;
    std::getline(inputFile, line);
    std::getline(inputFile, line);
    std::string startState;
    std::getline(inputFile, line);
    {
        std::istringstream iss(line);
        iss >> startState;
    }

    std::set<std::string> acceptStates;
    std::getline(inputFile, line);
    {
        std::istringstream iss(line);
        std::string state;
        while (iss >> state) {
            acceptStates.insert(state);
        }
    }

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
