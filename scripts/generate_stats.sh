#!/bin/bash
# 
# Given a corpus input file, generate unigram and ngrams tables.
# 
# Usage:
# ./generate_stats.sh INPUT_FILE OUTPUT_DIR
#
# A sample corpus is located at data/samples/big.txt.
#
# In the output folder, 4 files are created
# 
# unigram.txt
# ngrams2.ll
# ngrams3.ll
# ngrams4.ll
# 
# The file unigrams.txt contains lines of the form
# 
#   weight word
#   
# where 'weight' is simply the number of occurences of
# 'word' in the corpus.
# 
# The files ngramX.ll contain bi-, tri- and four-grams
# respectively. A line is of the form
# 
# word1<>word2<>...<>wordn<>rank weight
# 
# (rank is currently ignored).

INPUT_FILE=$1
OUTPUT_DIR=$2

echo "Generating unigrams frequency table"

NON_WORDS="[*.,?!;:#$%&()+/<>=@[]\^_{}|~\"][:blank:][:digit:]"

cat $INPUT_FILE |\
    tr $NON_WORDS "\n" |\
    tr 'A-Z' 'a-z' |\
    grep "^." |\
    sort -r -f |\
    uniq -c -i |\
    sort -nr > $OUTPUT_DIR/unigrams.txt

echo "Generating n-gram frequency tables"

for i in  {2..4}
do
    echo "Generating $i-gram frequency table..."
    count.pl --verbose --newline --token token_regexp_en.txt --ngram $i $OUTPUT_DIR/temp.cnt $INPUT_FILE
    #count.pl --verbose --newline --token --remove 2 TOKEN_REGEXP_EN.txt --ngram $i sample$i.cnt sample.txt
    statistic.pl --ngram $i -score 1.00 -frequency 1 ll.pm $OUTPUT_DIR/ngrams$i.ll $OUTPUT_DIR/temp.cnt
    #statistic.pl --ngram $i -score 1.00 -frequency 3 ll.pm sample$i.ll sample$i.cnt
    rm $OUTPUT_DIR/temp.cnt
done
