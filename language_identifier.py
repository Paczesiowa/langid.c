import sys
from base64 import b64decode
from bz2 import decompress
from cPickle import loads
from collections import defaultdict
from timeit import timeit

import numpy as np

from model import model


class LanguageIdentifier(object):

    def __init__(self, string):
        b = b64decode(string)
        z = decompress(b)
        model = loads(z)
        nb_ptc, nb_pc, self.nb_classes, self.tk_nextmove, self.tk_output = model
        self.nb_numfeats = int(len(nb_ptc) / len(nb_pc))
        self.nb_pc = np.array(nb_pc)
        self.nb_ptc = np.array(nb_ptc).reshape(self.nb_numfeats, len(self.nb_pc))

    def classify(self, text):
        text = map(ord, text)
        arr = np.zeros((self.nb_numfeats,), dtype='uint32')
        state = 0
        statecount = defaultdict(int)
        for letter in text:
            state = self.tk_nextmove[(state << 8) + letter]
            statecount[state] += 1
        for state in statecount:
            for index in self.tk_output.get(state, []):
                arr[index] += statecount[state]
        pdc = np.dot(arr, self.nb_ptc)
        probs = pdc + self.nb_pc
        cl = np.argmax(probs)
        conf = float(probs[cl])
        pred = str(self.nb_classes[cl])
        return pred, conf

    def split_models(self, path):
        with open(path + '/nb_ptc.bin', 'wb') as f:
            f.write(str(self.nb_ptc.shape[0]) + '\n')
            f.write(str(self.nb_ptc.shape[1]) + '\n')
            for i in xrange(self.nb_ptc.shape[0]):
                for j in xrange(self.nb_ptc.shape[1]):
                    f.write(str(self.nb_ptc[i, j]) + '\n')
        with open(path + '/nb_pc.bin', 'wb') as f:
            f.write(str(self.nb_pc.shape[0]) + '\n')
            for i in xrange(self.nb_pc.shape[0]):
                f.write(str(self.nb_pc[i]) + '\n')
        with open(path + '/nb_classes.bin', 'wb') as f:
            f.write(str(len(self.nb_classes)) + '\n')
            for i in xrange(len(self.nb_classes)):
                f.write(str(self.nb_classes[i]) + '\n')
        with open(path + '/tk_nextmove.bin', 'wb') as f:
            f.write(str(len(self.tk_nextmove)) + '\n')
            for i in xrange(len(self.tk_nextmove)):
                f.write(str(self.tk_nextmove[i]) + '\n')
        with open(path + '/tk_output.bin', 'wb') as f:
            max_key = max(self.tk_output.keys())
            max_value_len = max(map(len, self.tk_output.values()))
            f.write(str(max_key) + '\n')
            f.write(str(max_value_len) + '\n')
            for i in xrange(max_key + 1):  # inclusive of max key
                values = self.tk_output.get(i, [])
                for j in xrange(4):
                    try:
                        f.write(str(values[j]) + '\n')
                    except IndexError:
                        f.write('-1\n')

    def check_model(self):
        print self.nb_ptc.shape[0], self.nb_ptc.shape[1]
        for i in xrange(self.nb_ptc.shape[0]):
            for j in xrange(self.nb_ptc.shape[1]):
                print self.nb_ptc[i, j]
        print self.nb_pc.shape[0]
        for i in xrange(self.nb_pc.shape[0]):
            print self.nb_pc[i]
        print len(self.nb_classes)
        for i in xrange(len(self.nb_classes)):
            print self.nb_classes[i]
        print len(self.tk_nextmove)
        for i in xrange(len(self.tk_nextmove)):
            print self.tk_nextmove[i]
        for k in sorted(self.tk_output.keys()):
            t = self.tk_output[k]
            if len(t):
                print k,
                for x in t:
                    print x,
                print ''
        print self.nb_numfeats

    def check_output(self):
        text = 'quick brown fox jumped over the lazy dog'
        language, probability = self.classify(text)
        print ("The text '%s' has language %s (with probability %f)" % (
            text, language, probability))

    def benchmark(self):
        def foo():
            self.classify('quick brown fox jumped over the lazy dog')

        print '%.2fms' % round(timeit(foo, number=1000), 2)

if __name__ == '__main__':
    lid = LanguageIdentifier(model)
    if sys.argv[1] == 'check_model':
        lid.check_model()
    elif sys.argv[1] == 'check_output':
        lid.check_output()
    elif sys.argv[1] == 'split_models':
        lid.split_models('model')
    elif sys.argv[1] == 'benchmark':
        lid.benchmark()
