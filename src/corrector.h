/**
 * Copyright 2012 8pen
 *
 * A class for handling word variations used for spelling
 * corrections.
 */

#ifndef CORRECTOR_H
#define CORRECTOR_H

#include <string>
using namespace std;

struct string_pair {
    string first, second;
};

class Corrector {

public:
    static vector<string> variations(string word, vector<string> variations);

private:
    static string_pair createStringPair(string first, string second);
    static vector<string_pair> splits(string word, vector<string_pair> holder);
    static vector<string> deletes(vector<string_pair> splits, vector<string> holder);
    static vector<string> transposes(vector<string_pair> splits, vector<string> holder);
    static vector<string> replaces(vector<string_pair> splits, vector<string> holder);
    static vector<string> inserts(vector<string_pair> splits, vector<string> holder);
};

#endif