#ifndef HEAVY_TRACKER_HPP
#define HEAVY_TRACKER_HPP

#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <unordered_map>


struct TrackerUnit
{
    std::string realFP;      // Real fingerprint
    int realC = 0;           // Real counter
    std::string auxiliaryFP; // Auxiliary fingerprint
    int auxiliaryC = 0;      // Auxiliary counter
    bool tag = false;        // Tag: false => Mode A, true => Mode B
};

class HeavyTracker
{
private:
    double b, c, q, gammaVal, bHK; // parameters calculated by GA
    int D, M, THRESHOLD;

    // Random engine
    std::mt19937 gen;
    std::uniform_real_distribution<double> dist;

    std::vector<std::vector<TrackerUnit>> tracker;

    std::unordered_map<std::string, int> trueFrequency;

    size_t indexHash(const std::string &flowId, int layer);

public:
    HeavyTracker(int D_, int M_, int threshold_,
                 double b_, double c_, double q_, double gamma_, double bHK_);

    void modeA(const std::string &flowID, TrackerUnit &unit);

    void modeB(const std::string &flowID, TrackerUnit &unit);

    bool update(const std::string &flowID, int count = 1);

    const std::vector<std::vector<TrackerUnit>> &getTrackerRef() const
    {
        return tracker;
    }

      int calculateTrueHeavyHitters() const;

};

#endif
