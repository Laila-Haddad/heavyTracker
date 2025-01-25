#include "csv_reader.hpp"
using namespace std;

vector<tuple<string, string, int>> readCSV(const string &filename)
{
    vector<tuple<string, string, int>> data;
    ifstream file(filename);
    if(!file.is_open()){
        cerr<<"Failed to open file "<<filename<<"\n";
        return data;
    }
    string line;
    getline(file, line);

    while(getline(file, line)){
        istringstream ss(line);
        string src,dst,pkt;
        if(!getline(ss,src,',')) break;
        if(!getline(ss,dst,',')) break;
        if(!getline(ss,pkt,',')) break;
        data.emplace_back(src, dst, stoi(pkt));
    }
    file.close();
    return data;
}