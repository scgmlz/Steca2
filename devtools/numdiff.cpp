//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      devtools/numdiff.cpp
//! @brief     Implements namespace numdiff
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#include "devtools/numdiff.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <math.h>

namespace {
double relativeDiff(double x, double y)
{
    if (x==0 && y==0)
        return 0;
    return 2*fabs(x-y)/(x+y);
}

double compareLines(std::string line1, std::string line2)
{
    double tolerance = 0;

    std::vector<std::string> wordsOfLine1, wordsOfLine2;
    std::stringstream streamLine1;
    streamLine1 << line1;
    std::stringstream streamLine2;
    streamLine2 << line2;
    std::string word1, word2;
    while (std::getline(streamLine1, word1, ' '))
        wordsOfLine1.push_back(word1);
    while (std::getline(streamLine2, word2, ' '))
        wordsOfLine2.push_back(word2);

    if (wordsOfLine1.size() != wordsOfLine2.size())
        exit(1); //lines must have same number of words

    for (int i=0; i<wordsOfLine1.size(); i++) {
        try {
            //tries to handle words as double
            double value1 = std::stod(wordsOfLine1.at(i));
            double value2 = std::stod(wordsOfLine2.at(i));
            tolerance += relativeDiff(value1, value2);
        } catch (const std::invalid_argument& e) {
            //if not double, then compares as strings
            std::string word = wordsOfLine1.at(i);
            if (word.compare(wordsOfLine2.at(i)) != 0)
                exit(1);
        }
    }

    return tolerance;
}
} //namespace

bool numdiff::compareFiles(std::string filepath1, std::string filepath2, double maxtol)
{
    double tol = 0;

    std::ifstream in1(filepath1);
    std::ifstream in2(filepath2);
    if (!in1 || !in2)
        return false; //the two files must exist

    std::string ln1;
    std::string ln2;
    int x = 1; //wich line is read
    while(std::getline(in1, ln1) && std::getline(in2, ln2)) {
        double tolerance = compareLines(ln1, ln2);
        if (tolerance != 0)
            std::cout << tolerance << " in line " << x << std::endl;
        tol += tolerance;
        x++;
    }
    std::cout << "tol insg: " << tol << std::endl;

    return tol<maxtol;
}
