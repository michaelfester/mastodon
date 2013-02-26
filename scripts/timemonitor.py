#!/usr/bin/env python
#
# Copyright 2012 8pen

"""A time monitoring utitlity"""

import time

class TimeMonitor(object):  
    def start(self, msg=""):
        self.now = int(round(time.time() * 1000))
        self.msg = msg
        print msg

    def stop(self):
        before = self.now
        now = int(round(time.time() * 1000))
        print self.msg + " took " + str(now - before) + "ms"