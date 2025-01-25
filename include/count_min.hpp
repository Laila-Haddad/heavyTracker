#ifndef COUNT_MIN_SKETCH_HPP
#define COUNT_MIN_SKETCH_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <climits>

class CountMinSketch
{
private:
    int D;   
    int M;    
    std::vector<std::vector<int>> counts;
    std::vector<uint64_t> hashSeeds;   

public:
    CountMinSketch(int D_, int M_);

    void update(const std::string &flowID, int count = 1);

    int estimateCount(const std::string &flowID) const;

    bool isHeavyHitter(const std::string &flowID, int threshold) const {
        return (estimateCount(flowID) >= threshold);
    }
};

#endif
