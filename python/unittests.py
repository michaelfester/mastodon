#!/usr/bin/env python
#
# Copyright 2012 8pen

"""BinaryDictionary unit tests"""

import unittest
from bindict import BinaryDictionary
from trie import Trie
from operator import itemgetter

class DictionaryTest(unittest.TestCase):
    def setUp(self):
        self.unigrams = Trie()
        self.unigrams['a'] = 200
        self.unigrams['hi'] = 130
        self.unigrams['hello'] = 120
        self.unigrams['there'] = 140
        self.unigrams['how'] = 150
        self.unigrams['are'] = 80
        self.unigrams['you'] = 200
        self.unigrams['your'] = 100

        self.ngrams = Trie()
        self.ngrams[['hello','there']] = 20
        self.ngrams[['hello','you']] = 25
        self.ngrams[['how','are','you']] = 80
        self.ngrams[['you','are','there']] = 30
        self.ngrams[['are','you','there']] = 60

        self.bindict = BinaryDictionary()
        self.bindict.encode_unigrams(self.unigrams)
        self.bindict.encode_ngrams(self.ngrams)

    def test_trie_weight(self):
        self.assertEqual(self.unigrams['hello'], 120)
        self.assertEqual(self.ngrams[['hello','there']], 20)

    def test_trie_key_error(self):
        with self.assertRaises(KeyError):
            self.ngrams['hello']

    def test_trie_unigram_predict(self):
        self.assertTrue('e' in map(itemgetter(0), self.unigrams.get_predictions(['h'])))
        self.assertEquals('l', self.unigrams.get_predictions(list('he'))[0][0])
        self.assertEquals(len(self.unigrams.get_predictions(list('hello'))), 0)

    def test_trie_ngram_predict(self):
        self.assertTrue('there' in map(itemgetter(0), self.ngrams.get_predictions(['hello'])))
        self.assertTrue('you' in map(itemgetter(0), self.ngrams.get_predictions(['how','are'])))

    def test_bindict_exists(self):
        self.assertTrue(self.bindict.exists('hello'))
        self.assertTrue(not self.bindict.exists('hellos'))
        self.assertTrue(not self.bindict.exists('h'))
        self.assertTrue(self.bindict.exists('a'))

    def test_bindict_ngram_predict(self):
        self.assertTrue('there' in map(itemgetter(0), self.bindict.get_predictions(['hello'])))
        self.assertTrue('you' in map(itemgetter(0), self.bindict.get_predictions(['how','are'])))

    def test_correct(self):
        self.assertTrue('you' in self.bindict.get_corrections('yuu').keys())
        self.assertTrue('your' in self.bindict.get_corrections('yuur').keys())

    def test_completions(self):
        self.assertTrue('you' in self.bindict.get_completions('yo', 1))
        self.assertFalse('your' in self.bindict.get_completions('yo', 1))
        self.assertTrue('your' in self.bindict.get_completions('yo', 2))
        self.assertFalse('yo' in self.bindict.get_completions('y', 1))

if __name__ == '__main__':
    unittest.main()