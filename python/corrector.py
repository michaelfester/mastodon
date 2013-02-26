#!/usr/bin/env python
#
# Copyright 2012 8pen

"""Word corrector utilities"""

alphabet = 'abcdefghijklmnopqrstuvwxyz'

def variations(word):
    """Return a list of all variations of 'word' with edit
    distance 1.

    :param word: the word to vary
    """
    splits = [(word[:i], word[i:]) for i in range(len(word) + 1)]
    deletes = [a + b[1:] for a, b in splits if b]
    transposes = [a + b[1] + b[0] + b[2:] for a, b in splits if len(b) > 1]
    replaces = [a + c + b[1:] for a, b in splits for c in alphabet if b]
    inserts = [a + c + b for a, b in splits for c in alphabet]
    return set(deletes + transposes + replaces + inserts)
