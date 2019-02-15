#!/usr/bin/env python3

import math
import sys
import csv
import os

def is_line_required(line):
    if (
        line.startswith("FFBuild") or
        ("\"name\"" in line) or
        ("\"value\"" in line) or
        ("Final JPEG_Time" in line) or
        ("Final PNG_Time" in line)
    ):
        if "INFO -" not in line:
            return True
    return False

class PerfState:
    def __init__(self):
        self.timings = {}


def addTestValue(s, group, val):
    if group not in s.timings:
        s.timings[group] = []
    s.timings[group].append(float(val))

def handle_line(line, s):
	if "Capture_Time:" in line:
		fragment = line.split('Capture_Time:')[1].split('|')[0]
		group = fragment.split(',')[0]
		index = fragment.split(',')[1]
		val = fragment.split(',')[2]
		addTestValue(s, group, val)

	return s


def average(list):
	return float(sum(list))/len(list)


def median(list):
    n = len(list)
    if n < 1:
            return None
    if n % 2 == 1:
            return sorted(list)[n//2]
    else:
            return sum(sorted(list)[n//2-1:n//2+1])/2.0

def print_final_results(s):
	for group, times in s.timings.items():
		print("Group: " + group)

		print("Times: " + str(times))
		timeList = times[5:] #filter first 5
		print("Filtered Times: " + str(timeList))

		avg = average(timeList)
		print("Average: " + str("{0:,.2f}".format(avg)))

		variance = list(map(lambda x: (x - avg)**2, timeList))
		stdDev = math.sqrt(average(variance))
		print("StdDev: " + str("{0:,.2f}".format(stdDev)))

		m = median(timeList)
		print("Median: " + str("{0:,.2f}".format(m)))

		print()

def main():
    if len(sys.argv) < 2:
        print("Expected " + sys.argv[0] + " inputFileName")
        exit(1)
    argv1=sys.argv[1]
    s = PerfState()
    with open(argv1) as f:
        for line in f:
            s = handle_line(line, s)

    print_final_results(s)

main()
