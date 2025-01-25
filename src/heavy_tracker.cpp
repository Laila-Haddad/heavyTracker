#include "heavy_tracker.hpp"
using namespace std;

HeavyTracker::HeavyTracker(int D_, int M_, int threshold_,
                           double b_, double c_, double q_, double gamma_, double bHK_)
    : b(b_), c(c_), q(q_), gammaVal(gamma_), bHK(bHK_), D(D_), M(M_), THRESHOLD(threshold_)
{
    random_device rd;
    gen.seed(rd());
    dist = uniform_real_distribution<double>(0.0, 1.0);
    tracker.resize(D, vector<TrackerUnit>(M));
}

//hasg function, takes layer as the seed
size_t HeavyTracker::indexHash(const string &flowId, int layer)
{
    return (hash<string>{}(flowId) + 2654435789u * layer) % M;
}

// MODE A: Tag == false
void HeavyTracker::modeA(const string &flowID, TrackerUnit &unit)
{
    if (unit.realC == 0 || unit.realFP == flowID)
    {
        unit.realFP = flowID;
        unit.realC++;
        return;
    }

    if (unit.auxiliaryC == 0 || unit.auxiliaryFP == flowID)
    {
        unit.auxiliaryFP = flowID;
        unit.auxiliaryC++;
        return;
    }

    // Otherwise, exponential-expulsion on the aux slot
    double p_plus = (q / pow(b, (unit.auxiliaryC + c))) + gammaVal;
    double r = dist(gen);

    if (r < p_plus)
    {
        unit.auxiliaryFP = flowID;
        unit.auxiliaryC++;
    }

    // If auxC > realC, swap them
    if (unit.auxiliaryC > unit.realC)
    {
        swap(unit.realFP, unit.auxiliaryFP);
        swap(unit.realC, unit.auxiliaryC);
    }
}

// MODE B: Tag == true
void HeavyTracker::modeB(const string &flowID, TrackerUnit &unit)
{
    // if realC == 0 => start tracking new flow
    if (unit.realC == 0)
    {
        unit.realFP = flowID;
        unit.realC = 1;
        unit.auxiliaryC = 1;
        return;
    }

    // if same flow as the realFP => increment real + aux
    if (unit.realFP == flowID)
    {
        unit.realC++;
        unit.auxiliaryC++;
        return;
    }

    //  exponential-expulsion for realC
    {
        double p_plus = (q / pow(b, (unit.realC + c))) + gammaVal;
        double r = dist(gen);
        if (r < p_plus)
        {
            unit.realC++;
        }
    }

    // exponential-decay for auxiliaryC
    {
        double p_decay = 1.0 / pow(bHK, unit.auxiliaryC);
        double r2 = dist(gen);
        if (r2 < p_decay)
        {
            unit.auxiliaryC--;
            if (unit.auxiliaryC <= 0)
            {
                unit.realFP = flowID;
                unit.realC = 1;
                unit.auxiliaryC = 1;
            }
        }
    }
}


bool HeavyTracker::update(const string &flowID, int count)
{
    bool isHeavyHitter = false;
     trueFrequency[flowID] += count;

    for (int layer = 0; layer < D; ++layer)
    {
        size_t idx = indexHash(flowID, layer);
        auto &unit = tracker[layer][idx];

        if (unit.tag)
        {
            // If incoming flow matches the static fingerprint, its a heavy hitter
            if (unit.auxiliaryFP == flowID)
            {
                isHeavyHitter = true;
            }

            if (unit.realFP == flowID)
            {
                unit.realC += count;
                // if realC becomes higher than the threshold, its a heavy hitter
                if (unit.realC >= THRESHOLD)
                    isHeavyHitter = true;
            }
            else
            {
                for (int i = 0; i < count; i++)
                {
                    modeB(flowID, unit);
                }
                if (unit.realC >= THRESHOLD)
                    isHeavyHitter = true;
            }
        }
        else
        {
            // Tag == false => Mode A
            if (unit.realC < THRESHOLD)
            {
                for (int i = 0; i < count; i++)
                {
                    modeA(flowID, unit);
                }
                if (unit.realC >= THRESHOLD)
                {
                    unit.tag = true;
                    unit.auxiliaryFP = unit.realFP;
                    unit.realFP.clear();
                    unit.realC = 0;
                    unit.auxiliaryC = 0;
                    isHeavyHitter = true;
                }
            }
            else
            {
                // realC already >= threshold => we found a heavy hitter
                unit.tag = true;
                unit.auxiliaryFP = unit.realFP;
                unit.realFP.clear();
                unit.realC = 0;
                unit.auxiliaryC = 0;
                isHeavyHitter = true;
            }
        }
    }
    return isHeavyHitter;

    
}


int HeavyTracker::calculateTrueHeavyHitters() const
{
    int heavyHitterCount = 0;
    for (const auto &entry : trueFrequency)
    {
        if (entry.second > THRESHOLD)
        {
            ++heavyHitterCount;
        }
    }
    return heavyHitterCount;
}