
#ifndef GENETIC_ALGO_HPP
#define GENETIC_ALGO_HPP

#include <vector>
#include <string>
#include <tuple>
#include <random>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include "heavy_tracker.hpp"

struct Individual {
    std::string bits;
    double fitness;
};
extern const int    POP_SIZE;
extern const int    MAX_GENERATIONS;
extern const double CROSSOVER_RATE;
extern const double MUTATION_RATE;

extern const int    BITS_B;
extern const int    BITS_C;
extern const int    BITS_Q;
extern const int    BITS_GAMMA;
extern const int    CHROMO_LEN;

double decodeB(const std::string &bits);
double decodeC(const std::string &bits);
double decodeQ(const std::string &bits, double gammaVal);
double decodeGamma(const std::string &bits);

std::string randomBits(int length);

double evaluateFitness(Individual &ind,
                       const std::vector<std::tuple<std::string,std::string,int>> &dataset,
                       int M, int THRESHOLD);

// GA operators
std::vector<Individual> SUSselection(const std::vector<Individual> &population);
void crossover(Individual &p1, Individual &p2);
void mutate(Individual &ind);


#endif 

