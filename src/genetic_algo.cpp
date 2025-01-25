#include "genetic_algo.hpp"
using namespace std;

const int    POP_SIZE         = 40;
const int    MAX_GENERATIONS  = 30;
const double CROSSOVER_RATE   = 0.8;
const double MUTATION_RATE    = 0.02;

const int    BITS_B     = 8;
const int    BITS_C     = 8;
const int    BITS_Q     = 8;
const int    BITS_GAMMA = 8;
const int    CHROMO_LEN = BITS_B + BITS_C + BITS_Q + BITS_GAMMA;

static mt19937 GAgen(12345);
static uniform_real_distribution<double> u01(0.0,1.0);

double decodeB (const std::string &bits)
{
    int val=0;
    for(int i=0;i<BITS_B;i++){
        val = (val << 1) + (bits[i]=='1' ? 1 : 0);
    }
    double frac = val / double((1<<BITS_B)-1);
    return 1.0 + 0.1*frac; // => in [1, 1.1]
}

double decodeC (const std::string &bits)
{
    int offset = BITS_B;
    int val=0;
    for(int i=0; i<BITS_C; i++){
        val = (val << 1) + (bits[offset+i]=='1' ? 1 : 0);
    }
    double frac = val / double((1<<BITS_C)-1);
    return 10.0 * frac; // => in [0..10]
}

double decodeGamma(const std::string &bits)
{
    int offset = BITS_B + BITS_C + BITS_Q;
    int val=0;
    for(int i=0;i<BITS_GAMMA;i++){
        val = (val << 1) + (bits[offset + i]=='1' ? 1 : 0);
    }
    double frac = val / double((1<<BITS_GAMMA)-1);
    return 0.000001 + (0.001 - 0.000001)*frac; // => (0..0.001]
}

double decodeQ(const std::string &bits, double gammaVal)
{
    int offset = BITS_B + BITS_C;
    int val=0;
    for(int i=0;i<BITS_Q;i++){
        val = (val << 1) + (bits[offset + i]=='1' ? 1 : 0);
    }
    double frac = val / double((1<<BITS_Q)-1);
    // map to [0..(1 - gammaVal)]
    return frac * (1.0 - gammaVal);
}

std::string randomBits(int length){
    std::string s; 
    s.reserve(length);
    for(int i=0;i<length;i++){
        s.push_back(u01(GAgen)<0.5?'0':'1');
    }
    return s;
}

/** 
    fitness = - (1/m) * sum_{u=1..m} | nHat[u] - nReal[u] |
    where nReal[u] is the max frequency of any flow that hashed to unit u, 
    and nHat[u] is the final occupant's frequency in the tracker for unit u.
 */
double evaluateFitness(Individual &ind,
                       const vector<tuple<string,string,int>> &dataset,
                       int M, int THRESHOLD)
{
    //Decode parameters
    double gammaVal  = decodeGamma(ind.bits);
    double bVal      = decodeB(ind.bits);
    double cVal      = decodeC(ind.bits);
    double qVal      = decodeQ(ind.bits, gammaVal);

    //  single-layer for GA for easier error calculation
    int D = 1;

    //to calculate the true frequencies, to showcase the accuracy of the algotithm 
    vector<unordered_map<string,int>> trueCounts(M);

    HeavyTracker ht(D, M, THRESHOLD, bVal, cVal, qVal, gammaVal, bVal /*bHK*/);

    auto indexHashForGA = [&](const string &flowId){
        return (std::hash<string>{}(flowId)) % M;
    };

    for(const auto &row : dataset)
    {
        string flowID = get<0>(row) + ":" + get<1>(row);
        int count     = get<2>(row);

       
        size_t u = indexHashForGA(flowID); // hash to unit
        trueCounts[u][flowID] += count;  // update trueCounts
        ht.update(flowID, count);// update the tracker
    }

    // compute realMax[u] = maximum frequency in trueCounts[u]
    vector<int> realMax(M, 0);
    for (int u = 0; u < M; u++)
    {
        int mx = 0;
        for (auto &kv : trueCounts[u])
        {
            if (kv.second > mx) mx = kv.second;
        }
        realMax[u] = mx;
    }

    // calculating the AAE to define the fitness fucntion
    double sumAbsError = 0.0;

    // get reference to the single layer
    const auto &layer0 = ht.getTrackerRef()[0]; // D=1 so it's only one layer
    for (int u = 0; u < M; u++)
    {
        const auto &unit = layer0[u];
        int estimated = 0;
        if (!unit.tag) {
            // not reached threshold => occupant is realC
            estimated = unit.realC;
        }
        else {
            // once tag==true, there's definitely a flow >= threshold
            estimated = std::max(unit.realC, THRESHOLD);
        }

        int difference = abs(estimated - realMax[u]);
        sumAbsError += difference;
    }

    double avgAbsError = sumAbsError / double(M);
    double fitness = - avgAbsError;
    return fitness;
}


vector<Individual> SUSselection(const vector<Individual> &population)
{
    vector<Individual> newPop;
    newPop.reserve(population.size());

    //calculate the total fitness of the current population
    double totalFitness=0;
    for(const auto &ind : population){
        totalFitness += ind.fitness;
    }

    // the spacing between the pointers. must distribute the pointers evenly across the fitness range
    double spacing = totalFitness / population.size();
    // select a random point to start on. no individual is favored
    double start = u01(GAgen)*spacing; 

    double accum=0;
    int index=0;
    for(int i=0; i<(int)population.size(); i++){
        double pointer = start + i*spacing;
        //individuals are selected when the cumulative fitness surpasses a pointer.
        while(accum < pointer && index<(int)population.size()-1){
            accum += population[index].fitness;
            index++;
        }
        newPop.push_back(population[index]);
    }
    return newPop;
}

// two-point crossover
void crossover(Individual &p1, Individual &p2)
{
    if(u01(GAgen) < CROSSOVER_RATE){
        int c1 = uniform_int_distribution<int>(1, CHROMO_LEN-2)(GAgen);
        int c2 = uniform_int_distribution<int>(c1+1, CHROMO_LEN-1)(GAgen);
        for(int i=c1;i<=c2;i++){
            std::swap(p1.bits[i], p2.bits[i]);
        }
    }
}

// bit-flip mutation
void mutate(Individual &ind)
{
    for(int i=0;i<CHROMO_LEN;i++){
        if(u01(GAgen) < MUTATION_RATE){
            ind.bits[i] = (ind.bits[i] =='1' ? '0':'1');
        }
    }
}
