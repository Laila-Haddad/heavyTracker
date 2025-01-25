#include "genetic_algo.hpp"
#include "csv_reader.hpp"
#include "count_min.hpp"
#include "heavy_tracker.hpp"
#include <chrono>
#include <unordered_set>
#include <memory>

using namespace std;

const int TRAIN_THRESHOLD = 1000000;
const int TRAIN_M = 1000;

const string TRAIN_DATA = "train.csv";
const string TEST_DATA = "million.csv";
const int D = 3;

int getValidInput(const string &prompt, const string &errorPrompt, int recommendedValue) {
    int value;
    while (true) {
        cout << prompt << " (recommended: " << recommendedValue << "): ";
        cin >> value;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << errorPrompt << endl;
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
    }
}

unique_ptr<HeavyTracker> trainWithGA(int M, int threshold) {
    cout << "Training genetic algorithm..." << endl;

    auto trainData = readCSV(TRAIN_DATA); // read training data
    vector<Individual> population(POP_SIZE);

    for (auto &ind : population) {
        ind.bits = randomBits(CHROMO_LEN);
        ind.fitness = 0.0;
    }

    //evaluating fitness
    for (int generation = 0; generation < MAX_GENERATIONS; generation++) {
        for (auto &ind : population) {
            ind.fitness = evaluateFitness(ind, trainData, M, threshold);
        }

        sort(population.begin(), population.end(),
             [](const Individual &a, const Individual &b) { return a.fitness > b.fitness; });

        cout << "Generation " << generation << " bestFitness = " << population[0].fitness << endl;

        vector<Individual> newPop = SUSselection(population);

        for (size_t i = 0; i + 1 < newPop.size(); i += 2) {//crossover
            crossover(newPop[i], newPop[i + 1]);
        }

        for (auto &ind : newPop) {//mutation
            mutate(ind);
        }

        population = move(newPop);
    }

    const auto &best = population.front(); //getting the best individual in the population
    double gamma = decodeGamma(best.bits);
    double b = decodeB(best.bits);
    double c = decodeC(best.bits);
    double q = decodeQ(best.bits, gamma);

    cout << "GA Results: b = " << b << ", c = " << c
         << ", q = " << q << ", gamma = " << gamma
         << ", fitness = " << best.fitness << endl;

    return make_unique<HeavyTracker>(D, M, threshold, b, c, q, gamma, b);//start the heavy tracker with the found parametets
}

    
int main()
{

    //user input: if GA training is wanted, and what are the prefered test threshold and M
    unique_ptr<HeavyTracker> heavyTracker;
    char withGa;

    cout << "Would you like to get count-min and heavTracker results with predefined parameters (the alternetive is training GA first and using its parameters for heavytracker)? y/n" << endl;
    cin >> withGa;
    
    if (withGa != 'y' && withGa != 'n')
       { cout << "Error input. Enter y or n only." << endl;
    return 1;}

        int TEST_THRESHOLD = getValidInput(
        "Enter the wanted threshold",
        "Invalid input. Please enter a valid number.",
        1000000
    );

    // Prompt for row length
    int M = getValidInput(
        "Enter the wanted row length for each layer",
        "Invalid input. Please enter a valid number.",
        10000
    );


    if (withGa == 'n')
    {
       heavyTracker = trainWithGA(TRAIN_M, TEST_THRESHOLD);
    }

    else if (withGa == 'y')
    {
        heavyTracker = make_unique<HeavyTracker>(D, M, TEST_THRESHOLD, 1.02196, 8.35294, 0.32548, 3.23412e-05, 1.02196);
    }

    // comparing heavytracker and count min.
    string testFile = TEST_DATA;
    auto testData = readCSV(testFile);

    CountMinSketch cm(D, M);

    unordered_set<string> HTheavyReport;
    unordered_map<string, int> trueFreq;

    for (const auto &row : testData)
    {
        string flowID = get<0>(row) + ":" + get<1>(row); // create the flowID
        int pktCount = get<2>(row);

        bool isHeavy = heavyTracker->update(flowID, pktCount); // update heavytracker
        if (isHeavy)
        {
            HTheavyReport.insert(flowID);
        }

        cm.update(flowID, pktCount); // update Count-Min Sketch

        trueFreq[flowID] += pktCount; // track the reacl frrequency
    }

    int CMheavyCount = 0;
    for (auto &kv : trueFreq)
    {
        string flow = kv.first;

        int estC = cm.estimateCount(flow); // find the flow count from CM
        if (estC >= TRAIN_THRESHOLD)
        {
            CMheavyCount++;
        }
    }

    int trueHeavyHitters = heavyTracker->calculateTrueHeavyHitters();

    cout << "HeavyTracker detected " << HTheavyReport.size() << " heavy hitters.\n\n";
    cout << "CountMinSketch detected " << CMheavyCount << " heavy hitters.\n\n";
    cout << "True number of heavy hitters: " << trueHeavyHitters << "\n";

    return 0;
}
