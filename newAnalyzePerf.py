#!/usr/bin/env python3

import math
import sys
import csv
import os
from urllib.parse import urlparse
from statistics import mean, median, stdev

class PerfState:
    def __init__(self):
        self.timings = {}

def addTestValue(s, group, val):
    if group not in s.timings:
        s.timings[group] = []
    s.timings[group].append(float(val))

def handle_line(line, s, skipFirstHost):
    if "Capture_Time:" in line:
        fragment = line.split('Capture_Time:')[1].split('|')[0]
        group = fragment.split(',')[0]
        if skipFirstHost:
            group = group.split('(')[1].split(')')[0]
            u = urlparse(group)
            if u.scheme == "data":
                return s
            u = u._replace(netloc=u.netloc.split('.', 1)[1])
            group = u.geturl()
        index = fragment.split(',')[1]
        val = fragment.split(',')[2]
        addTestValue(s, group, val)
    return s

def print_final_results(s):
    print('{ "data" : [')
    first = True
    for group, times in s.timings.items():
        if first:
            first = False
        else:
            print(',')
        print("{")

        print('  "Group": "' + group + '",')

        print('  "Times": ' + str(times) + ",")
        timeList = times[5:] #filter first 5
        print('  "Filtered Times": ' + str(timeList) + ",")

        avg = mean(timeList)
        print('  "Average": "' + str("{0:,.2f}".format(avg)) + '",')

        stdDev = stdev(timeList)
        print('  "StdDev": "' + str("{0:,.2f}".format(stdDev)) + '",')

        m = median(timeList)
        print('  "Median": "' + str("{0:,.2f}".format(m)) + '"')

        print("}")

    print("]}")


def main():
    if len(sys.argv) < 2:
        print("Expected " + sys.argv[0] + " inputFileName [RemoveFirstSubdomain]")
        exit(1)
    inputFileName = sys.argv[1]
    skipFirstHost = False
    if len(sys.argv) >= 3:
        skipFirstHost = bool(sys.argv[2])
    s = PerfState()
    with open(inputFileName) as f:
        for line in f:
            s = handle_line(line, s, skipFirstHost)

    print_final_results(s)

main()
