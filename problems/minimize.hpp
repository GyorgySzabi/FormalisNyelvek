#ifndef PROBLEMS_MINIMIZE_H
#define PROBLEMS_MINIMIZE_H

#include "../problem.hpp"

class MinimizeProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif 
