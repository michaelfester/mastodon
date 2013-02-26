/**
 * Copyright 2012 8pen
 *
 * BinaryDictionary unit tests.
 */

#include <UnitTest++.h>
#include <algorithm>
#include <vector>
#include "../../bindict.h"

struct DictionaryTestFixture {
    BinaryDictionary bindict;

    /**
     * The test dictionary was generated using
     * python/makedict.py with the -g option.
     *
     * It holds the following tries:
     *
     * unigrams = Trie()
     * unigrams['a'] = 200
     * unigrams['hi'] = 130
     * unigrams['hello'] = 120
     * unigrams['there'] = 140
     * unigrams['how'] = 150
     * unigrams['are'] = 80
     * unigrams['you'] = 200
     * unigrams['your'] = 100

     * ngrams = Trie()
     * ngrams[['hello','there']] = 20
     * ngrams[['hello','you']] = 25
     * ngrams[['how','are','you']] = 80
     * ngrams[['you','are','there']] = 30
     * ngrams[[are','you',there']] = 30
     */
    DictionaryTestFixture() {
        bindict.fromFile("../dictionaries/test/test.dict");
    }

    ~DictionaryTestFixture() {}
};

TEST_FIXTURE(DictionaryTestFixture, TestLoad) {
    CHECK(bindict.isLoaded());
}

TEST_FIXTURE(DictionaryTestFixture, TestExists) {
    CHECK(bindict.exists("hello"));
    CHECK(bindict.exists("a"));
    CHECK(!bindict.exists("bonjour"));
    CHECK(!bindict.exists("h"));
}

TEST_FIXTURE(DictionaryTestFixture, TestNgramPredict) {
    string phrase[] = { "hello" };    
    vector<weighted_string> holder;
    vector<weighted_string> predictions = bindict.getPredictions(phrase, 1, holder, 4);
    int numPredictions = predictions.size();
    CHECK_EQUAL(numPredictions, 2);
    
    // Dirty. Use boost.
    string predictionStrings[numPredictions];
    for (int i = 0; i < numPredictions; i++) {
        predictionStrings[i] = predictions[i].value;
    }

    CHECK((int) count(predictionStrings, predictionStrings+numPredictions, "there") > 0);
    CHECK_EQUAL((int) count(predictionStrings, predictionStrings+numPredictions, "blah"), 0);

    string phrase2[] = { "how", "are" };
    holder.clear();
    predictions.clear();
    predictions = bindict.getPredictions(phrase2, 2, holder, 4);
    numPredictions = predictions.size();
    CHECK_EQUAL(numPredictions, 1);
    
    // Dirty. Use boost.
    string predictionStrings2[numPredictions];
    for (int i = 0; i < numPredictions; i++) {
        predictionStrings2[i] = predictions[i].value;
    }
    
    CHECK((int) count(predictionStrings, predictionStrings + numPredictions, "you") > 0);
    CHECK_EQUAL((int) count(predictionStrings, predictionStrings + numPredictions, "blah"), 0);
}

TEST_FIXTURE(DictionaryTestFixture, TestCorrect) {
    vector<weighted_string> holder;
    vector<weighted_string> corrections = bindict.getCorrections("you", holder, 100);
    int numCorrections = corrections.size();
    CHECK_EQUAL(numCorrections, 1);

    // Dirty. Use boost.
    string correctionStrings[numCorrections];
    for (int i = 0; i < numCorrections; i++) {
        correctionStrings[i] = corrections[i].value;
    }

    CHECK((int) count(correctionStrings, correctionStrings + numCorrections, "you") > 0);

    holder.clear();
    corrections.clear();
    corrections = bindict.getCorrections("yuu", holder, 100);
    numCorrections = corrections.size();
    CHECK_EQUAL(numCorrections, 1);

    // Dirty. Use boost.
    string correctionStrings2[numCorrections];
    for (int i = 0; i < numCorrections; i++) {
        correctionStrings2[i] = corrections[i].value;
    }

    CHECK((int) count(correctionStrings2, correctionStrings2 + numCorrections, "you") > 0);
    CHECK_EQUAL((int) count(correctionStrings2, correctionStrings2 + numCorrections, "yuu"), 0);
}

// TODO:
// TEST_FIXTURE(DictionaryTestFixture, test_completions) {
//     self.assertTrue('you' in self.bindict.get_completions('yo', 1))
//     self.assertFalse('your' in self.bindict.get_completions('yo', 1))
//     self.assertTrue('your' in self.bindict.get_completions('yo', 2))
//     self.assertFalse('yo' in self.bindict.get_completions('y', 1))
// }

int main() {
    return UnitTest::RunAllTests();
}