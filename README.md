# Mastodon

A simple next-word prediction engine

## Quick start

```
# Fetch a sample corpus
mkdir data; mkdir data/samples
curl http://norvig.com/big.txt -o data/samples/big.txt

# Generate stats using NSP
mkdir data/output; cd scripts
./generate_stats.sh ../data/samples/big.txt ../data/output/

# Create binary dictionaries
cd ..; mkdir dictionaries; mkdir dictionaries/test; cd python
python makedict.py -u ../data/output/unigrams.txt -n ../data/output/ngrams2.ll,../data/output/ngrams3.ll,../data/output/ngrams4.ll -o ../dictionaries/test/big.dict

# Create binary dictionaries for unit tests
python makedict.py -t
python unittests.py
cd ../cpp
make test
```

## Generating statistics

To create a binary dictionary, we need data created from the N-Gram Statistics Package (NSP), available at http://www.d.umn.edu/~tpederse/nsp.html. The script `generate_stats.sh` in the `scripts/` folder serves this purpose.

A sample corpus can be found at https://dl.dropbox.com/u/228601/8pen/big.txt.


```
curl https://dl.dropbox.com/u/228601/8pen/big.txt -o data/samples/big.txt
```

We can generate the desired statistics in the following way:

```
cd scripts
./generate_stats.sh INPUT_FILE OUTPUT_DIR
```

### Unigrams

The script generates a simple word frequency list `unigram.txt` in `OUTPUT_DIR`, in which each line is of the form `weight unigram`. Example output:

```
79377 the
39997 of
38076 and
28604 to
21780 in
20910 a
...
```

The weight is simply the number of occurences of the corresponding word in the corpus.

### N-grams

The script then generates a lists of bi-, tri-, and four-grams (`ngrams2.ll`, `ngrams3.ll`, `ngrams4.ll`, also locaed in `OUTPUT_DIR`) of the form `unigram<>unigram<>...<>rank weight` (we ignore `rank` for now). Example output:

```
of<>the<>2 25053.6988
in<>the<>6 10335.9606
did<>not<>8 9798.6723
```

## Generating dictionaries

To generate a binary dictionary using output of the NSP, a script `makedict.py` in the `python/` folder is available. Example usage:

```
python makedict.py -u UNIGRAM_FILE -n BIGRAM_FILE,TRIGRAM_FILE,FOURGRAM_FILE -o OUTPUT_FILE
```

## Using dictionaries

Implementations in Python and C++ are currently available for loading a binary dictionary and querying it for:

* Corrections
* Completions (Python only)
* Next-word predictions

### Python

Here is a simple usage in Python:

```
bindict = BinaryDictionary.from_file('../dictionaries/test/test.dict')
bindict.get_predictions(['hello']) # => [('there',10),('sir',3)]
bindict.get_corrections('yuur')    # => ['your','you','year']
bindict.get_completions('yo', 2)   # => ['you','your']
```

### C++

Here is a simple usage in C++:

```
BinaryDictionary bindict;
bindict.fromFile("../dictionaries/test/test.dict");

string phrase[] = {"how", "are"};
vector<weighted_string> holder;
vector<weighted_string> predictions = bindict.getPredictions(phrase, 2, holder, 4);

vector<weighted_string> holder;
vector<weighted_string> corrections = bindict.getCorrections("you", holder, 100);
```

Note that querying for word completions is not yet implemented in C++.

## Unit tests

The unit tests are designed to be used with a simple dictionary, located at `dictionaries/test/test.dict`, and generated using the `-t` option:
```
python makedict.py -t
```

### Python

The Python unit tests use the `unittest` module, and are available in `python/unittests.py`:
```
python unittests.py
```

### C++

The C++ unit tests, located at `cpp/tests/unit/test.cpp`, are based on the `UnitTest++` framework (included). Simply use the provided `Makefile` in the `cpp` folder to run the tests:
```
make test
```

## Generating statistics

## License

Mastodon is released under the MIT license. See [LICENSE.md](LICENSE.md).