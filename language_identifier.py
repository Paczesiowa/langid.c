import sys
from base64 import b64decode
from bz2 import decompress
from cPickle import loads
from collections import defaultdict
from datetime import datetime
from multiprocessing import Process
from subprocess import Popen
from time import sleep
from urlparse import parse_qs

import fapws._evwsgi as evwsgi
import numpy as np
import requests
import zmq
from fapws import base

from model import model


http_server_address = 'http://127.0.0.1:10000/'
zmq_server_address = 'tcp://127.0.0.1:5555'
run_count = 5000


def server_http():
    lid = LanguageIdentifier(model)
    headers = [('Content-type', 'text/javascript; charset=utf-8')]
    status = '200 OK'

    def application(environ, start_response):
        params = parse_qs(environ['QUERY_STRING'])
        text = params['q'][0]
        normalize = params['normalize'][0] == 'true'
        pred, conf = lid.classify(text, normalize=normalize)
        start_response(status, headers)
        return [pred + ' ' + str(conf)]

    evwsgi.start('127.0.0.1', '10000')
    evwsgi.set_base_module(base)
    evwsgi.wsgi_cb(('', application))
    evwsgi.set_debug(0)
    evwsgi.run()


def server_zmq():
    lid = LanguageIdentifier(model)
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.bind(zmq_server_address)
    while True:
        message = socket.recv()
        normalize = message[0] == '1'
        text = message[1:]
        language, probability = lid.classify(text, normalize=normalize)
        socket.send(bytes(language) + b' ' + '%.2f' % probability)


class LanguageIdentifier(object):

    def __init__(self, string):
        b = b64decode(string)
        z = decompress(b)
        model = loads(z)
        nb_ptc, nb_pc, self.nb_classes, self.tk_nextmove, self.tk_output = \
            model
        self.nb_numfeats = int(len(nb_ptc) / len(nb_pc))
        self.nb_pc = np.array(nb_pc)
        self.nb_ptc = np.array(nb_ptc).reshape(self.nb_numfeats,
                                               len(self.nb_pc))

    def classify(self, text, normalize=False):
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
        if normalize:
            probs = 1 / np.exp(probs[None, :] - probs[:, None]).sum(1)
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
        language, probability = self.classify(text, normalize=True)
        print ("The text '%s' has language %s (with norm. probability %f)" % (
            text, language, probability))

    def benchmark(self):
        t0 = datetime.now()
        for _ in xrange(run_count):
            self.classify('quick brown fox jumped over the lazy dog')
        t1 = datetime.now()
        elapsed = (t1 - t0).total_seconds() * (1000000. / run_count)
        print '%d microseconds per run' % elapsed

        t0 = datetime.now()
        for _ in xrange(run_count):
            self.classify('quick brown fox jumped over the lazy dog',
                          normalize=True)
        t1 = datetime.now()
        elapsed = (t1 - t0).total_seconds() * (1000000. / run_count)
        print '%d microseconds per normalized run' % elapsed

    def check_http_output(self):
        p = Process(target=server_http)
        p.start()
        sleep(3)

        try:
            text = 'quick brown fox jumped over the lazy dog'
            language, probability = self.classify(text, normalize=False)
            expected_output = language + ' ' + str(probability)

            params = {'q': text, 'normalize': 'false'}
            resp = requests.get(http_server_address, params=params).text
            if resp != expected_output:
                print 'Wrong output:', resp,
                print '(should be:', expected_output + ')'

            language, probability = self.classify(text, normalize=True)
            expected_output = language + ' ' + str(probability)
            params = {'q': text, 'normalize': 'true'}
            resp = requests.get(http_server_address, params=params).text
            if resp != expected_output:
                print 'Wrong normalized output:', resp,
                print '(should be:', expected_output + ')'
        finally:
            p.terminate()

    def benchmark_http(self, keep_alive=False):
        p = Process(target=server_http)
        p.start()
        sleep(3)
        s = requests.Session() if keep_alive else requests

        try:
            text = 'quick brown fox jumped over the lazy dog'
            params = {'q': text, 'normalize': 'false'}
            t0 = datetime.now()
            for _ in xrange(run_count):
                s.get(http_server_address, params=params)
            t1 = datetime.now()
            elapsed = (t1 - t0).total_seconds() * (1000000. / run_count)
            if keep_alive:
                msg = '%d microseconds per http call (with Keep-Alive)'
            else:
                msg = '%d microseconds per http call'
            print msg % elapsed

            params = {'q': text, 'normalize': 'true'}
            t0 = datetime.now()
            for _ in xrange(run_count):
                s.get(http_server_address, params=params)
            t1 = datetime.now()
            elapsed = (t1 - t0).total_seconds() * (1000000. / run_count)

            if keep_alive:
                msg = ('%d microseconds per normalized ' +
                       'http call (with Keep-Alive)')
            else:
                msg = '%d microseconds per normalized http call'
            print msg % elapsed
        finally:
            p.terminate()

    def check_zmq_output(self, python_server):
        if python_server:
            p = Process(target=server_zmq)
            p.start()
        else:
            p = Popen(['./language_identifier_zmq_server'])
        sleep(3)

        try:
            text = 'quick brown fox jumped over the lazy dog'
            language, probability = self.classify(text, normalize=False)
            expected_output = language + ' ' + '%.2f' % probability

            context = zmq.Context()
            socket = context.socket(zmq.REQ)
            socket.connect(zmq_server_address)

            msg = '0' + text
            socket.send(msg)
            resp = socket.recv()
            if resp != expected_output:
                print 'Wrong output:', resp,
                print '(should be:', expected_output + ')'

            language, probability = self.classify(text, normalize=True)
            expected_output = language + ' ' + '%.2f' % probability
            msg = '1' + text
            socket.send(msg)
            resp = socket.recv()
            if resp != expected_output:
                print 'Wrong normalized output:', resp,
                print '(should be:', expected_output + ')'
        finally:
            if python_server:
                p.terminate()
            else:
                p.kill()

    def benchmark_zmq(self, python_server):
        if python_server:
            p = Process(target=server_zmq)
            p.start()
        else:
            p = Popen(['./language_identifier_zmq_server'])
        sleep(3)

        try:
            context = zmq.Context()
            socket = context.socket(zmq.REQ)
            socket.connect(zmq_server_address)

            text = 'quick brown fox jumped over the lazy dog'
            msg = '0' + text

            t0 = datetime.now()
            for _ in xrange(run_count):
                socket.send(msg)
                socket.recv()
            t1 = datetime.now()
            elapsed = (t1 - t0).total_seconds() * (1000000. / run_count)
            print '%d microseconds per 0mq call' % elapsed

            msg = '1' + text
            t0 = datetime.now()
            for _ in xrange(run_count):
                socket.send(msg)
                socket.recv()
            t1 = datetime.now()
            elapsed = (t1 - t0).total_seconds() * (1000000. / run_count)
            print '%d microseconds per normalized 0mq call' % elapsed
        finally:
            if python_server:
                p.terminate()
            else:
                p.kill()

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
    elif sys.argv[1] == 'check_http_output':
        lid.check_http_output()
    elif sys.argv[1] == 'benchmark_http':
        lid.benchmark_http(keep_alive=False)
        lid.benchmark_http(keep_alive=True)
    elif sys.argv[1] == 'check_zmq_output':
        lid.check_zmq_output(python_server=sys.argv[2] == 'python_server')
    elif sys.argv[1] == 'benchmark_zmq':
        lid.benchmark_zmq(python_server=sys.argv[2] == 'python_server')
