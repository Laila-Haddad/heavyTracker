#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <fstream>
#include <sstream>

std::vector<std::tuple<std::string, std::string, int>> readCSV(const std::string &filename);

#endif
