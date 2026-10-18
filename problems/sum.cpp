#include <string>
#include <fstream>
#include <iostream>

#include "sum.hpp"


void SumProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("sum", "Sum the numbers in the input file");
}

bool SumProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("sum") > 0;
}

int SumProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);

    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    int sum = 0;
    int number;

    while (inputFile >> number) {
        sum += number;
    }

    std::ofstream outputFile(outputFilename);

    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return 1;
    }

    outputFile << sum;
    return 0;
}