#!/usr/bin/env python
#
# Copyright 2012 8pen

"""A binary unigram and ngram dictionary."""

import math
from collections import defaultdict
import corrector
import byteutils

CACHE_ENABLED = True

class BinaryDictionary(object):
    """A binary dictionary of unigrams and ngrams,
    represented as a byte array.

    The byte array is split into two parts: unigrams and
    ngrams. Unigrams start at index 0.
    
    ========================================================
    Unigram header
    --------------------------------------------------------
    0,1,2   : num children
    3,4,5   : bigrams address
    ========================================================
    Unigram nodes
    --------------------------------------------------------
    0       : char
    1       : weight
    2       : num children
    3,4,5   : parent node address
    6,7,8   : child1 address
    9,10,11 : child2 address
    ...     : childn address
    ========================================================
    N-gram header
    --------------------------------------------------------
    0,1,2   : num children
    ========================================================
    N-gram nodes
    --------------------------------------------------------
    0,1,2   : unigram address (i.e. address of tail node
              of a word in unigram trie)
    3       : weight
    4       : num children
    5,6,7   : child1 address
    8,9,10  : child2 address
    ...     : childn address
    """

    def __init__(self):
        self.pos = 6
        self.word_cache = {}
        self.ngram_cache = {}
        self.bytes = bytearray(24*1024*1024)
        self.ngrams_offset = -1

    @staticmethod
    def from_file(filename):
        """Return a binary dictionary instance from a file

        :param filename: the file to read the dictionary from
        """
        d = BinaryDictionary()
        with open (filename, 'r') as file:
            d.bytes = bytearray(file.read())
        return d


    def write_to_file(self, filename):
        """Write the dictionary to a file.

        :param filename: the output filename where the
                         dictionary should be written to
        """
        f = open(filename,"wb")
        trimmed = bytearray()
        pos = 0
        chunk_size = 2048
        while pos < self.pos:
            trimmed.extend(self.bytes[pos:pos+chunk_size])
            pos += chunk_size
        f.write(trimmed)
        f.close()

    def encode_unigrams(self, root_node):
        """Serialize the unigram trie into the byte array

        :param node: the root node of the unigram trie
        """
        num_nodes = len(root_node)
        self.bytes[0] = 0xff & (num_nodes >> 16)
        self.bytes[1] = 0xff & (num_nodes >> 8)
        self.bytes[2] = 0xff & num_nodes
        # Reserved for the ngrams offset
        self.bytes[3] = 0
        self.bytes[4] = 0
        self.bytes[5] = 0
        self.pos = 6
        self.__add_unigram_node(root_node, chr(0), 0)

    def __add_unigram_node(self, node, value, parent_address):
        """Add a unigram node to the byte array

        :param node: the unigram node in the trie
        :param value: the node's (char) value
        :param parent_address: the parent node's address in the byte array
        """
        children = node.path.keys()
        offset = self.pos
        self.bytes[offset] = value
        self.bytes[offset+1] = min(255, int(node.value)) if node.value else 0
        if node.value and int(node.value) > 5000:
            #print "High freq node found at offset " + str(offset) + ", " + str(value)
            pass
        self.bytes[offset+2] = len(children)
        self.bytes[offset+3] = 0xff & (parent_address >> 16)
        self.bytes[offset+4] = 0xff & (parent_address >> 8)
        self.bytes[offset+5] = 0xff & parent_address
        c = 0
        offset_children = offset + 6
        self.pos = offset_children + 3*len(children)
        for key in node.path:
            child_pos = self.__add_unigram_node(node.path[key], key, offset)
            self.bytes[offset_children+3*c] = 0xff & (child_pos >> 16)
            self.bytes[offset_children+3*c+1] = 0xff & (child_pos >> 8)
            self.bytes[offset_children+3*c+2] = 0xff & child_pos
            c += 1
        return offset

    def encode_ngrams(self, root_node):
        """Serialize the ngram trie into the byte array.

        :param node: the root node of the ngram trie
        """
        num_nodes = len(root_node)
        self.bytes[3] = 0xff & (self.pos >> 16)
        self.bytes[4] = 0xff & (self.pos >> 8)
        self.bytes[5] = 0xff & self.pos
        self.bytes[self.pos] = 0xff & (num_nodes >> 16)
        self.bytes[self.pos+1] = 0xff & (num_nodes >> 8)
        self.bytes[self.pos+2] = 0xff & num_nodes
        self.pos += 3
        self.__add_ngram_node(root_node, None)#, 0)

    def __add_ngram_node(self, node, word):
        """Add an ngram node to the byte array.

        :param node: the node in the trie object to add
        :param word: the value of the node
        """
        children = node.path.keys()
        offset = self.pos
        unigram_tail_pos = self.__get_unigram(word) if word else 0
        self.bytes[offset] = 0xff & (unigram_tail_pos >> 16)
        self.bytes[offset+1] = 0xff & (unigram_tail_pos >> 8)
        self.bytes[offset+2] = 0xff & unigram_tail_pos
        self.bytes[offset+3] = min(255, int(math.floor(float(node.value)))) if node.value else 0
        self.bytes[offset+4] = min(255, len(children))
        c = 0
        offset_children = offset + 5
        self.pos = offset_children + 3*len(children)
        for key in node.path:
            child_pos = self.__add_ngram_node(node.path[key], key)#, offset)
            self.bytes[offset_children+3*c] = 0xff & (child_pos >> 16)
            self.bytes[offset_children+3*c+1] = 0xff & (child_pos >> 8)
            self.bytes[offset_children+3*c+2] = 0xff & child_pos
            c += 1
        return offset

    def __get_unigrams_offset(self):
        """Return the position, in the byte array, of the first
        unigram node"""
        return 6

    def __get_ngrams_offset(self):
        """Return the position, in the byte array, of the first
        ngram node"""
        if self.ngrams_offset < 0:
            self.ngrams_offset = byteutils.to_int(self.bytes, 3, 3)
        return self.ngrams_offset

    def __is_final_unigram(self, node):
        """Return true if the node is a final node, that is,
        it has positive weight.

        :param node: a unigram node
        """
        return self.__unigram_weight(node) > 0

    def __unigram_weight(self, node):
        """Return the weight of a unigram node

        :param node: a unigram node
        """
        return self.bytes[node+1]

    def __ngram_weight(self, node):
        """Return the weight of an ngram node

        :param node: an ngram node
        """
        return self.bytes[node+3]

    def __get_unigram(self, word, offset=6, prefix=""):
        """Return the address of the final node in a word, or 0 if not found

        :param word: the word to look up
        """
        if CACHE_ENABLED:
            if word and word in self.word_cache:
                return self.word_cache[word]

        if len(word) == 0:
            if len(prefix) > 0:
                if CACHE_ENABLED:
                    self.word_cache[prefix] = offset
                return offset
            return 0
        
        head = word[0]
        num_children = self.bytes[offset+2]
        if num_children == 0:
            return 0
        for i in range(0,num_children):
            child_pos = byteutils.to_int(self.bytes, offset + 6 + 3*i, 3)
            if chr(self.bytes[child_pos]) == head:
                return self.__get_unigram(word[1:len(word)], child_pos,
                    prefix + head)
        return 0

    def __get_unigrams(self, words):
        """Return the unigram nodes of the words in the list

        :param words: a list of words
        """
        return [self.__get_unigram(word) for word in words]

    def __get_ngram(self, unigrams, offset=-1, prefix=[]):
        """Given a chain of unigrams, return the address
        of the corresponding ngram, i.e. of the last node in this
        chain.

        :param unigrams: a list of unigram addresses pointing
        to the words in the phrase.
        """
        offset = self.__get_ngrams_offset() + 3 if offset == -1 else offset
        if CACHE_ENABLED:
            k = self.__get_ngram_key(unigrams)
            if k and k in self.ngram_cache:
                return self.ngram_cache[k]

        if len(unigrams) == 0:
            if len(prefix) > 0:
                if CACHE_ENABLED:
                    self.ngram_cache[self.__get_ngram_key(prefix)] = offset
                return offset
            return 0

        head = unigrams[0]
        num_children = self.bytes[offset+4]
        if num_children == 0:
            return 0
        for i in range(0,num_children):
            child_pos = byteutils.to_int(self.bytes, offset + 5 + 3*i, 3)
            child_unigram_pos = byteutils.to_int(self.bytes, child_pos, 3)
            if child_unigram_pos == head:
                prefix.append(head)
                return self.__get_ngram(unigrams[1:len(unigrams)], child_pos,
                    prefix)
        return 0

    def __get_ngram_key(self, unigrams):
        """Return the key, in ngram_cache, of a list of unigrams. The ngram_cache
        stores the address of an ngram, if found, stemming from a list of unigrams.
        Thus, when looking of an ngram given a list of unigrams, we don't need
        to traverse the trie each time.

        :param unigram: a list of unigrams
        """
        key = ""
        for unigram in unigrams:
            key += str(unigram) + "_"
        return key

    def __get_unigram_children(self, unigram, limit=20):
        """Return a list of tuples of the form (child_address, weight),
        where child_address is the address of a child to the given
        unigram node, and weight its weight. Order by decreasing weight.

        :param unigram: the parent unigram node address

        :param limit: the maximum number of addresses to return
        """
        offset_num_children = 2
        offset_children_address = 6
        offset_weight = 1
        num_children = self.bytes[unigram + offset_num_children]
        children = []
        for i in range(0, num_children):
            child_address = byteutils.to_int(self.bytes, unigram + offset_children_address + 3*i, 3)
            child_weight = self.bytes[child_address + offset_weight]
            children.append((child_address, child_weight))
        return sorted(children, key=lambda c: c[1], reverse=True)

    def __get_ngram_children(self, ngram, limit=20):
        """Same as __get_unigram_children(), but looking at the
        ngrams trie instead.
        """
        offset_num_children = 4
        offset_children_address = 5
        offset_weight = 3
        num_children = self.bytes[ngram + offset_num_children]
        children = []
        for i in range(0, num_children):
            child_address = byteutils.to_int(self.bytes, ngram + offset_children_address + 3*i, 3)
            child_weight = self.bytes[child_address + offset_weight]
            children.append((child_address, child_weight))
        return sorted(children, key=lambda c: c[1], reverse=True)

    def __get_unigram_from_ngram(self, ngram):
        """Given a node in the ngram trie, return the address
        of the unigram that it points to.

        :param ngram_node: the node address in the ngram trie
        """
        return byteutils.to_int(self.bytes, ngram, 3)

    def __get_ancestors(self, node):
        """Return a list of ancestors of a given unigram node, where the
        first element is the node itself, and the last is a root node.

        :param node: the address of the last child node in the chain
        """
        ancestors = [node]
        parent = self.__get_parent(node)
        while parent > self.__get_unigrams_offset():
            ancestors.insert(0, parent)
            parent = self.__get_parent(parent)
        return ancestors

    def __get_parent(self, node):
        """Given a node in the unigram trie, return its parent.

        :param node: the child node
        """
        if node <= 0 or node >= self.__get_ngrams_offset():
            return 0
        return byteutils.to_int(self.bytes, node + 3, 3)

    def __get_descendants(self, node, depth):
        """Given a unigram node, return the tree below that node,
        i.e. all the nodes which are final, and have node as
        ancestor.

        :param node: the parent node
        :param depth: the seek depth, that is, the number of
        generations below node to look for
        """
        if depth == 0:
            return [node] if self.__unigram_weight(node) > 0 else []
        completions = []
        children = self.__get_unigram_children(node)
        if len(children) == 0:
            return [node]
        for (child, weight) in children:
            if weight > 0:
                completions.append(child)
            completions += self.__get_descendants(child, depth-1)
        return list(set(completions))

    def __construct_word(self, nodes):
        """Given a list of (unigram) nodes, reconstruct the
        corresponding word. NB: no check is made as to whether the
        addresses are valid, and whether subsequent elements in the
        list are parents/children to each other.

        :param node_list: a list of nodes, where the first element
        corresponds to the first character of the word (i.e. it's a
        root node) and the last is a tail node
        """
        if not nodes:
            return ""
        word = ""
        for node in nodes:
            char_value = self.bytes[node]
            if char_value == 0:
                continue
            word += str(unichr(char_value))
        return word

    def __known_variations(self, word):
        """For a given word, return a list of known words obtained as variations, and
        variations thereof, of the word, as defined in the 'corrector' module

        :param word: the word whose variations we're looking up
        """
        return set(e2 for e1 in corrector.variations(word) for e2 in corrector.variations(e1) if self.exists(e2))

    def __known(self, words):
        """Filter a list of words to only include known words

        :param words: a list a words
        """
        known = {}
        for word in words:
            unigram = self.__get_unigram(word)
            if unigram == 0:
                continue
            weight = self.__unigram_weight(unigram)
            if weight > 0:
                known[word] = weight
        return known

    def exists(self, word):
        """Determine wether a word is present in the unigram trie.

        :param word: the word to look up in the unigram trie
        """
        unigram = self.__get_unigram(word)
        return self.__is_final_unigram(unigram) if unigram > 0 else False

    def get_predictions(self, words):
        """Return a list of weighted next word predictions.

        :param words: a list of words representing a phrase, e.g. ['how','are']

        :return a list of pairs of the form (word, weight), ordered
        by decreasing weight
        
        TODO: pass max number of desired completions
        """

        unigrams = self.__get_unigrams(words)
        ngram = self.__get_ngram(unigrams)
        children = self.__get_ngram_children(ngram)
        predictions = []
        for child in children:
            unigram = self.__get_unigram_from_ngram(child[0])
            ancestors = self.__get_ancestors(unigram)
            word = self.__construct_word(ancestors)
            predictions.append((word, child[1]))
        return predictions

    def get_corrections(self, word):
        """Get spelling corrections of a word, using simple substitutions,
        transposes, inserts etc. a la Peter Norvig. For instance,

        'yuu' => [{'you':200}, {'your':120}]

        param word: the word to correct

        TODO: pass max number of desired completions
        """
        return self.__known([word]) or self.__known(corrector.variations(word)) or self.__known_variations(word) or [word]

    def get_completions(self, word, depth):
        """Return a list of completions of a given word. For instance,
        'yo' => ['you', 'your']

        :param word: the word to complete

        :param depth: the number of characters to look ahead for. For instance:

            get_completions('yo', 2) => ['you', 'your']
            get_completions('yo', 1) => ['you']

        TODO: pass max number of desired completions
        """
        node = self.__get_unigram(word)
        completions = self.__get_descendants(node, depth)
        words = []
        for unigram in completions:
            ancestors = self.__get_ancestors(unigram)
            w = self.__construct_word(ancestors)
            words.append(w)
        return words

    def get_suggestions(self, word, depth):
        """TODO"""
        pass