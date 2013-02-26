/**
 * Copyright 2012 8pen
 *
 * A binary dictionary of unigrams and ngrams.
 */

#include <string>
#include <iostream>
#include <fstream>
#include "bindict.h"
#include "corrector.h"

using namespace std;

#define DEBUG false
#define CACHE_ENABLED true
#define MAX_WORD_LENGTH 48

/**
 * Read a binary dictionary file into the byte array.
 * @param filename the path to the binary dictionary file
 */
void BinaryDictionary::fromFile(const char * filename) {
    ifstream file (filename, ios::in|ios::binary|ios::ate);
    if (file.is_open()) {
        size = file.tellg();
        bytes = new char [size];
        file.seekg(0, ios::beg);
        file.read(bytes, size);
        file.close();
        if (DEBUG) {
            cout << "Loaded " << size << " bytes " << endl;
        }
        loaded = true;
    } else {
        if (DEBUG) {
            cout << "Unable to open file";
        }
    }
}

weighted_string BinaryDictionary::createWeightedString(string value, int weight) {
    weighted_string word;
    word.value = value;
    word.weight = weight;
    return word;
}

/**
 * Determine whether a word is present in the unigram trie.
 * @param word the word to look up in the unigram trie
 * @return true if the word is present in the unigram trie
 */
bool BinaryDictionary::exists(string word) {
    int unigram = getUnigram(word);
    if (unigram == 0) {
        return false;
    } else {
        return isFinalUnigram(unigram);
    }
}

/**
 * Get the weighted next word predictions predictions of an ngram.
 * @param words a list of words constituting the ngram
 * @param numWords the number of words in the ngram
 * @param predictions an empty holder to fill up with predictions
 * @param numPredictions the maximum number of desired predictions
 * @return the number of predictions found, but at most numPredictions
 */
vector<weighted_string> BinaryDictionary::getPredictions(string* words, int numWords, vector<weighted_string> predictions, int maxPredictions) {
    int unigrams[numWords];
    getUnigrams(words, unigrams, numWords);
    int ngram = getNgram(unigrams, numWords);
    weighted_int children[maxPredictions];
    int numChildren = getNgramChildren(ngram, children, maxPredictions);
    for (int i = 0; i < numChildren; i++) {
        int unigram = getUnigramFromNgram(children[i].value);
        int ancestors[MAX_WORD_LENGTH];
        int numAncestors = getAncestors(unigram, ancestors);
        string word = constructWord(ancestors, numAncestors);
        weighted_string prediction = BinaryDictionary::createWeightedString(word, children[i].weight);
        predictions.push_back(prediction);
    }
    return predictions;
}

/**
 * Get spelling corrections of a word, using simple substitutions,
 * transposes, inserts etc. a la Peter Norvig. For instance,
 * 
 * 'yuu' => [{'you':200}, {'your':120}]
 * 
 * @param word the word to correct
 * @param corrections the list of corrections
 * @param maxCorrections the maximum number of desired corrections
 * 
 * @return the number of corrections found, but at most maxCorrections
 */
vector<weighted_string> BinaryDictionary::getCorrections(string word, vector<weighted_string> corrections, int maxCorrections) {
    if (maxCorrections == 0) return corrections;

    try {
        weighted_string ww = getWeightedWord(word);
        if (ww.weight > 0) {
            corrections.push_back(ww);
            return corrections;
        }
    } catch (int i) {
    }

    // Corrections of edit distance 1
    vector<string> variations;
    variations = Corrector::variations(word, variations);
    corrections = known(variations, corrections);

    if (corrections.size() > 0) {
        return corrections;
    }

    // TODO
    // Corrections of edit distance 2

    return corrections;
}

// TODO
// weighted_string[] BinaryDictionary::getCompletions(string word, int depth) {}

// TODO
// weighted_string[] BinaryDictionary::getSuggestions(string word, int depth) {}

/**
 * Return the position, in the byte array, of the first unigram node.
 * @return the position of the first unigram node
 */
int BinaryDictionary::getUnigramsOffset() {
    return 6;
}

/**
 * Return the position, in the byte array, of the n-grams header.
 * Note that the first actual n-gram node starts three positions
 * after, the first three bytes being reserved to the number of
 * child nodes.
 * @return the position of the first ngram node
 */
int BinaryDictionary::getNgramsOffset() {
    if (ngramsOffset < 0) {
        ngramsOffset = toInt(bytes, 3, 3);
    }
    return (unsigned char) ngramsOffset;
}

/**
 * Determine whether a unigram node is final, that is, has positive weight.
 * @param node a unigram node
 * @return true if the node has positive weight
 */
bool BinaryDictionary::isFinalUnigram(int node) {
    return getUnigramWeight(node) > 0;
}

/**
 * Return the weight of a unigram node.
 * @param node a unigram node
 * @return the weight of the unigram node
 */
int BinaryDictionary::getUnigramWeight(int node) {
    return (unsigned char) bytes[node+1];
}

/**
 * Return the weight of an ngram node
 * @param node an ngram node
 * @return the weight of the node
 */
int BinaryDictionary::getNgramWeight(int node) {
    return (unsigned char) bytes[node+3];
}

/**
 * Return the address of the final node in a word, or 0 if not found.
 * @param word the word to look up
 * @return the address of the final node in the word
 */
int BinaryDictionary::getUnigram(string word) {
    return getUnigram(word, 0, getUnigramsOffset(), word);
}

/**
 * Return the address of the final node in a word, or 0 if not found
 * @param prefix the word prefix (used for recursion)
 * @param suffix the word to look up
 * @param offset the offset in the byte array (= 6 for root node)
 * @return the address of the final node in the word
 */
int BinaryDictionary::getUnigram(string word, int prefixSize, int offset, string cacheKey) {
    if (CACHE_ENABLED) {
        It it = unigramCache.find(cacheKey);
        if (it != unigramCache.end()) {
            return it->second;
        }
    }

    int length = word.length();
    if (length == 0) {
        if (prefixSize > 0) {
            if (CACHE_ENABLED) {
               unigramCache[cacheKey] = offset;
            }
            return offset;
        }
        return 0;
    }

    char head = word[0];
    int numChildren = (unsigned char) bytes[offset+2];
    if (numChildren == 0) {
        return 0;
    }
    for (int i = 0; i < numChildren; i++) {
        int childPos = toInt(bytes, offset + 6 + 3*i, 3);
        if ((unsigned char) bytes[childPos] == head) {
            return getUnigram(word.substr(1, length), prefixSize + 1, childPos, cacheKey);
        }
    }

    return 0;
}

/**
 * Return the addresses of a list of words.
 * @param words a list of words
 * @param an empty holder for unigram addresses
 * @param size the number of words in the list
 * @return the number of unigrams found (currently just equal to size)
 */
int BinaryDictionary::getUnigrams(string* words, int* unigrams, int size) {
    for (int i = 0; i < size; i++) {
        unigrams[i] = getUnigram(words[i]);
    }
    return size;
}

/**
 * Given a chain of unigrams, return the address of the corresponding ngram,
 * i.e. of the last node in this chain.
 * @param unigrams a list of unigram addresses pointing to the words in the phrase.
 * @param size the size of the unigrams array
 * @return the address of the corresponding ngram
 */
int BinaryDictionary::getNgram(int* unigrams, int size) {
    return getNgram(unigrams, size, 0, getNgramsOffset() + 3, getNgramCacheKey(unigrams, size));
}

/**
 * Cf. getNgram(int[] unigrams, int size)
 */
int BinaryDictionary::getNgram(int* unigrams, int unigramsSize, int prefixSize, int offset, string cacheKey) {
    if (CACHE_ENABLED) {
        It it = ngramCache.find(cacheKey);
        if (it != ngramCache.end()) {
            return it->second;
        }
    }

    if (unigramsSize == 0) {
        if (prefixSize > 0) {
            if (CACHE_ENABLED) {
               ngramCache[cacheKey] = offset;
            }
            return offset;
        }
        return 0;
    }

    int head = unigrams[0];
    int numChildren = (unsigned char) bytes[offset + 4];
    if (numChildren == 0) {
        return 0;
    }

    for (int i = 0; i < numChildren; i++) {
        int childPos = toInt(bytes, offset + 5 + 3*i, 3);
        int childUnigramPos = toInt(bytes, childPos, 3);
        if ((unsigned char) childUnigramPos == head) {
            return getNgram(unigrams + 1, unigramsSize - 1, prefixSize + 1, childPos, cacheKey);
        }
    }
    return 0;
}

/**
 * Return the ngram cache key corresponding to a list of unigrams.
 * 
 * @param unigrams the unigram list
 * @param size the number of unigrams in the list
 * @return the key used in the ngram cache
 */
string BinaryDictionary::getNgramCacheKey(int* unigrams, int size) {
    string s = "";
    for (int i = 0; i < size; i++) {
        s.append("" + unigrams[i]);
    }
    return s;
}

/**
 * Return a list of tuples of the form (child_address, weight),
 * where child_address is the address of a child to the given
 * node, and weight its weight. Order by decreasing weight.
 * @param node the parent node address
 * @param children a holder for the result
 * @param limit the maximum number of addresses to return
 * @return the number of children, but not exceeding limit
 */
int BinaryDictionary::getUnigramChildren(int unigram, weighted_int* children, int limit) {
    int numChildren = (unsigned char) bytes[unigram + 2];
    int size = min(numChildren, limit);
    for (int i = 0; i < size; i++) {
        int childAddress = toInt(bytes, unigram + 6 + 3*i, 3);
        int childWeight = bytes[childAddress + 1];
        weighted_int node;
        node.value = childAddress;
        node.weight = childWeight;
        children[i] = node;
    }
    return size;
    // TODO: sort result
    // return sorted(children, key=lambda c: c[1], reverse=True);
}

/**
 * Same as getUnigramChildren(), but looking at the
 * ngrams trie instead.
 * @return the number of children, but not exceeding limit
 */
int BinaryDictionary::getNgramChildren(int ngram, weighted_int* children, int limit) {
    int numChildren = (unsigned char) bytes[ngram + 4];
    int size = min(numChildren, limit);
    for (int i = 0; i < size; i++) {
        int childAddress = toInt(bytes, ngram + 5 + 3*i, 3);
        int childWeight = bytes[childAddress + 3];
        weighted_int node;
        node.value = childAddress;
        node.weight = childWeight;
        children[i] = node;
    }
    return size;
    // TODO: sort result
    // return sorted(children, key=lambda c: c[1], reverse=True);
}

/**
 * Given a node in the ngram trie, return the address
 * of the unigram that it points to.
 * @param ngram the node address in the ngram trie
 * @return the address of the unigram it points to
 */
int BinaryDictionary::getUnigramFromNgram(int ngram) {
    return toInt(bytes, ngram, 3);
}

/**
 * Return a list of ancestors of a given unigram node, where the
 * first element is the node itself, and the last is a root node.
 * @param node the address of the last child node in the chain
 * @return the number of ancestors, including the node itself
 */
int BinaryDictionary::getAncestors(int node, int* ancestors) {
    ancestors[0] = node;

    int parent = getParent(node);
    if (parent == 0) {
        return 1;
    }

    int numAncestors = 1;
    while (parent > getUnigramsOffset()) {
        ancestors[numAncestors] = parent;
        parent = getParent(parent);
        numAncestors++;
    }

    return numAncestors;
}

/**
 * Given a node in the unigram trie, return its parent.
 * @param node the child node
 * @return the parent node address
 *
 * Note: no safety check is made, so node could potentially
 * be the address of an ngram node, in which case the
 * return value is wrong.
 */
int BinaryDictionary::getParent(int node) {
    if (node <= 0) {
        return 0;
    }
    return toInt(bytes, node + 3, 3);
}

// TODO
// int BinaryDictionary::getDescendants(int node, int* completions, int depth, int count) {}

// TODO
// int BinaryDictionary::getDescendants(int node, int* completions, int depth, int count) {}

/**
 * Given a list of (unigram) nodes, reconstruct the
 * corresponding word. NB: no check is made as to whether the
 * addresses are valid, and whether subsequent elements in the
 * list are parents/children to each other.
 * @param nodeList: a list of nodes, where the first element
 * corresponds to the first character of the word (i.e. it's a
 * root node) and the last is a tail node
 * @return the reconstructed word
 */
string BinaryDictionary::constructWord(int* nodeList, int numNodes) {
    string word = "";
    for (int i = 0; i < numNodes; i++) {
        int value = bytes[nodeList[i]];
        if (value == 0) continue;
        // Look into wchar_t and wstring for wider strings
        // http://stackoverflow.com/questions/2940681/c-chr-and-unichr-equivalent
        word = ((char) value) + word;
    }
    return word;
}

weighted_string BinaryDictionary::getWeightedWord(string word) {
    int unigram = getUnigram(word);
    if (unigram == 0) throw 0;
    weighted_string ww;
    ww.value = word;
    ww.weight = getUnigramWeight(unigram);
    return ww;
}

// TODO
// string[] BinaryDictionary::knownVariations(int word) {}

/**
 * Filter a list of words to only include known words.
 *
 * @param words a list a words
 * @param filtered the filtered list
 * @param numWords the number of words in the list
 * @return numFiltered the number of elements remaining
 */
vector<weighted_string> BinaryDictionary::known(vector<string> words, vector<weighted_string> filtered) {
    int count = 0;
    for (int i = 0; i < words.size(); i++) {
        try {
            weighted_string ww = getWeightedWord(words[i]);
            if (ww.weight == 0) {
                continue;
            }
            filtered.push_back(ww);
        } catch (int i) {
            continue;
        }
    }
    return filtered;
}