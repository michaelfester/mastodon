#!/usr/bin/env python
#
# Copyright 2012 8pen

"""Binary dictionary generator"""

import sys
import getopt
from timemonitor import TimeMonitor
from trie import Trie
from bindict import BinaryDictionary

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "h:o:u:n:dt")
    except getopt.error, msg:
        print msg
        print "Usage: 'python makedict.py -u unigrams -n bigrams,trigrams,fourgrams -o output'"
        print "Debug: 'python makedict.py -d'"
        print "Generate test dict: 'python makedict.py -t'"
        sys.exit(2)

    unigrams = []
    ngrams = []
    output = ""
    for o,l in opts:
        if "-t" == o:
            generate_test_dict()
            return
        if "-d" == o:
            unigrams = ['../data/output/unigrams.txt']
            ngrams = ['../data/output/ngrams2.ll']
            output = '../dictionaries/test/debug.dict'
            break
        if "-u" == o:
            unigrams = [l]
        if "-n" == o:
            ngrams = l.split(',')
        if "-o" == o:
            output = l
      
    if not output:
        print "No output file specified"
        sys.exit(2)
    if len(unigrams) == 0 and len(ngrams) == 0:
        print "Must at least specify one source for either unigrams or n-grams"
        sys.exit(2)

    monitor = TimeMonitor()
    monitor.start("Creating tries")
    if len(unigrams) > 0:
        unigrams = Trie.from_files(unigrams)
    if len(ngrams) > 0:
        ngrams = Trie.from_files(ngrams, True)
    monitor.stop()

    monitor.start("Encoding to binary dictionary")
    d = BinaryDictionary()
    print "Encoding unigrams..."
    d.encode_unigrams(unigrams)
    print "Encoding ngrams..."
    d.encode_ngrams(ngrams)
    print "Writing file to " + str(output)
    d.write_to_file(output)
    monitor.stop()

def generate_test_dict():
    unigrams = Trie()
    unigrams['a'] = 200
    unigrams['hi'] = 130
    unigrams['hello'] = 120
    unigrams['there'] = 140
    unigrams['how'] = 150
    unigrams['are'] = 80
    unigrams['you'] = 200
    unigrams['your'] = 100

    ngrams = Trie()
    ngrams[['hello','there']] = 20
    ngrams[['hello','you']] = 25
    ngrams[['how','are','you']] = 80
    ngrams[['you','are','there']] = 30
    ngrams[['are','you','there']] = 60

    bindict = BinaryDictionary()
    bindict.encode_unigrams(unigrams)
    bindict.encode_ngrams(ngrams)

    bindict.write_to_file('../dictionaries/test/test.dict')

if __name__ == "__main__":
    main()