#ifndef PROBLEMS_DETERMINIZE_H
#define PROBLEMS_DETERMINIZE_H

#include "../problem.hpp"

class DeterminizeProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // PROBLEMS_DETERMINIZE_H
