/**
 * Copyright 2012 8pen
 *
 * A class for handling word variations used for spelling
 * corrections.
 */

#include <iostream>
#include <string>
#include <vector>
#include "corrector.h"

using namespace std;

string_pair Corrector::createStringPair(string first, string second) {
    string_pair pair;
    pair.first = first;
    pair.second = second;
    return pair;
}

/**
 * Return a vector of all variations of 'word' with edit
 * distance 1.
 * 
 * @param word the word to vary
 * @param variations a holder for the vector of variations
 * @return the variations of 'word'
 */
vector<string> Corrector::variations(string word, vector<string> variations) {
    vector<string_pair> holder;
    vector<string_pair> splits = Corrector::splits(word, holder);
    vector<string> holderDeletes;
    vector<string> deletes = Corrector::deletes(splits, holderDeletes);
    vector<string> holderTransposes;
    vector<string> transposes = Corrector::transposes(splits, holderTransposes);
    vector<string> holderReplaces;
    vector<string> replaces = Corrector::replaces(splits, holderReplaces);
    vector<string> holderInserts;
    vector<string> inserts = Corrector::inserts(splits, holderInserts);

    variations.reserve(deletes.size() + transposes.size() + replaces.size() + inserts.size());
    variations.insert(variations.end(), deletes.begin(), deletes.end());
    variations.insert(variations.end(), transposes.begin(), transposes.end());
    variations.insert(variations.end(), replaces.begin(), replaces.end());
    variations.insert(variations.end(), inserts.begin(), inserts.end());

    return variations;
}

vector<string_pair> Corrector::splits(string word, vector<string_pair> holder) {
    int l = word.length();
    for (int i = 0; i <= l; i++) {
        string_pair pair = Corrector::createStringPair(word.substr(0,i), word.substr(i,l));
        holder.push_back(pair);
    }
    return holder;
}

vector<string> Corrector::deletes(vector<string_pair> splits, vector<string> holder) {
    for (vector<string_pair>::iterator it = splits.begin(); it < splits.end(); it++) {
        if ((*it).second.length() == 0) break;
        holder.push_back((*it).first + (*it).second.substr(1,(*it).second.length()));
    }
    return holder;
}

vector<string> Corrector::transposes(vector<string_pair> splits, vector<string> holder) {
    for (vector<string_pair>::iterator it = splits.begin(); it < splits.end(); it++) {
        if ((*it).second.length() <= 1) break;
        holder.push_back((*it).first + (*it).second[1] + (*it).second[0] + (*it).second.substr(2,(*it).second.length()));
    }
    return holder;
}

vector<string> Corrector::replaces(vector<string_pair> splits, vector<string> holder) {
    string ALPHABET = "abcdefghijklmnopqrstuvwxyz";
    for (vector<string_pair>::iterator it = splits.begin(); it < splits.end(); it++) {
        if ((*it).second.length() == 0) break;
        for (int i = 0; i < ALPHABET.length(); i++) {
            holder.push_back((*it).first + ALPHABET[i] + (*it).second.substr(1,(*it).second.length()));
        }
    }
    return holder;
}

vector<string> Corrector::inserts(vector<string_pair> splits, vector<string> holder) {
    string ALPHABET = "abcdefghijklmnopqrstuvwxyz";
    for (vector<string_pair>::iterator it = splits.begin(); it < splits.end(); it++) {
        for (int i = 0; i < ALPHABET.length(); i++) {
            holder.push_back((*it).first + ALPHABET[i] + (*it).second);
        }
    }
    return holder;
}