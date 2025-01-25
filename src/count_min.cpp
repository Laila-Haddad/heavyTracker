#include "count_min.hpp"
using namespace std;

static inline uint64_t hashStringWithSeed(const std::string &str, uint64_t seed)
{
    unsigned long hash = seed;
    for (auto c : str)
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

CountMinSketch::CountMinSketch(int D_, int M_)
    : D(D_), M(M_)
{
    counts.resize(D, std::vector<int>(M, 0));

    std::random_device rd;
    for (int i = 0; i < D; i++)
    {
        hashSeeds.push_back(((uint64_t)rd() << 32) ^ (uint64_t)rd());
    }
}

void CountMinSketch::update(const std::string &flowID, int count)
{
    for (int i = 0; i < D; i++)
    {
        // compute hash index in [0..M-1]
        uint64_t hv = hashStringWithSeed(flowID, hashSeeds[i]);
        size_t idx = (size_t)(hv % M);
        // add the count
        counts[i][idx] += count;
    }
}

int CountMinSketch::estimateCount(const std::string &flowID) const
{
    int ret = INT_MAX;
    for (int i = 0; i < D; i++)
    {
        uint64_t hv = hashStringWithSeed(flowID, hashSeeds[i]);
        size_t idx = (size_t)(hv % M);
        //choose minimum counter
        ret = std::min(ret, counts[i][idx]);
    }
    return ret;
}

