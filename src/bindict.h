/**
 * Copyright 2012 8pen
 *
 * A binary dictionary of unigrams and ngrams.
 */

#ifndef BINDICT_H
#define BINDICT_H

#include <string>
#include <iostream>
#include <fstream>
#include <tr1/unordered_map>
#include <vector>
using namespace std;

typedef std::tr1::unordered_map<string, int> Dict;
typedef Dict::const_iterator It;


// TODO:
// Use Boost tuples instead
// http://staticallytyped.wordpress.com/2011/05/07/c-structs-vs-tuples-or-why-i-like-tuples-more/
//
struct weighted_string {
    string value;
    int weight;
};

struct weighted_int {
    int value;
    int weight;
};

/**
 * A binary dictionary consists of a byte array serializing
 * two tries: a unigram trie and an ngram trie. Unigrams
 * fill up the first part of the array, ngrams fill up the
 * rest.
 *
 * A node in the ngram trie consists of a pointer to the
 * tail node in the unigram trie of the corresponding word.
 * Thus, the ngrams contain only words that are found in
 * the unigram trie.
 * 
 * 
 * ========================================================
 * Unigram header
 * --------------------------------------------------------
 * 0,1,2   : number of children
 * 3,4,5   : address of the ngram header below
 * ========================================================
 * Unigram nodes
 * --------------------------------------------------------
 * 0       : char
 * 1       : weight
 * 2       : number of children nodes
 * 3,4,5   : parent node address
 * 6,7,8   : child1 address
 * 9,10,11 : child2 address
 * ...     : childn address
 * ========================================================
 * N-gram header
 * --------------------------------------------------------
 * 0,1,2   : number of children nodes
 * ========================================================
 * N-gram nodes
 * --------------------------------------------------------
 * 0,1,2   : unigram address (i.e. address of tail node
 *           of a word in unigram trie)
 * 3       : weight
 * 4       : number of children nodes
 * 5,6,7   : child1 address
 * 8,9,10  : child2 address
 * ...     : childn address
 */

class BinaryDictionary {

private:
    ifstream::pos_type size;
    char * bytes;
    bool loaded;
    Dict unigramCache;
    Dict ngramCache;
    int ngramsOffset;

    int getUnigramsOffset();
    int getNgramsOffset();
    bool isFinalUnigram(int node);
    int getUnigramWeight(int node);
    int getNgramWeight(int node);
    int getUnigram(string word);
    weighted_string getWeightedWord(string word);
    int getUnigram(string word, int prefixSize, int offset, string cacheKey);
    int getUnigrams(string* words, int* unigrams, int size);
    int getNgram(int* unigrams, int size);
    int getNgram(int* unigrams, int unigramsSize, int prefixSize, int offset, string cacheKey);
    string getNgramCacheKey(int* unigrams, int size);
    int getUnigramChildren(int unigram, weighted_int* children, int limit);
    int getNgramChildren(int ngram, weighted_int* children, int limit);
    int getUnigramFromNgram(int ngram);
    int getAncestors(int node, int* ancestors);
    int getParent(int node);
    // TODO:
    // int getDescendants(int node, int* completions, int depth);
    string constructWord(int* nodeList, int numNodes);
    // string[] knownVariations(int word);
    vector<weighted_string> known(vector<string> words, vector<weighted_string> filtered);
    static weighted_string createWeightedString(string value, int weight);

public:
    int toInt(char * byteArray, int offset, int chunkSize) {
        int value = 0;
        for (int i = 0; i < chunkSize; i++) {
            value += (unsigned char) byteArray[offset + i] << (chunkSize-i-1)*8;
        }
        return value;
    }

    bool isLoaded() { return loaded; }
    BinaryDictionary() { ngramsOffset = -1; }
    ~BinaryDictionary() { delete[] bytes; }

    void fromFile(const char * filename);
    bool exists(string word);
    vector<weighted_string> getPredictions(string* words, int numWords, vector<weighted_string> predictions, int maxPredictions);
    vector<weighted_string> getCorrections(string word, vector<weighted_string> corrections, int maxCorrections);
    // TODO:
    // int getCompletions(string word, int depth);
};

#endif