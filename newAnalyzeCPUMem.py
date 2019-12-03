#!/usr/bin/env python3

import math
import sys
import csv
import os
from urllib.parse import urlparse

class SiteState:
    def __init__(self, site):
        self.site = site
        self.cgmem = []
        self.mem = []
        self.cpu0 = []
        self.cpu1 = []
        self.cpu2 = []
        self.cpu3 = []

class PerfState:
    def __init__(self):
        self.sites = {}
        self.current_site = ''

    def set_site(self, site):
        if site not in self.sites:
            self.sites[site] = SiteState(site)
        self.current_site = site

    def add_cgmem(self, cgmem):
        self.sites[self.current_site].cgmem.append(int(cgmem))
    def add_mem(self, mem):
        self.sites[self.current_site].mem.append(int(mem))
    def add_cpu0(self, cpu0):
        self.sites[self.current_site].cpu0.append(float(cpu0))
    def add_cpu1(self, cpu1):
        self.sites[self.current_site].cpu1.append(float(cpu1))
    def add_cpu2(self, cpu2):
        self.sites[self.current_site].cpu2.append(float(cpu2))
    def add_cpu3(self, cpu3):
        self.sites[self.current_site].cpu3.append(float(cpu3))


def handle_line(line, s):
    if 'Site : ' in line:
        site = line.split('Site : ')[1].strip()
        s.set_site(site)
    elif 'CGMem: ' in line:
        cgmem = line.split('CGMem: ')[1].strip()
        s.add_cgmem(cgmem)
    elif 'Mem: ' in line:
        mem = line.split('Mem: ')[1].strip()
        s.add_mem(mem)
    elif 'CPU 0   ' in line:
        cpu0 = line.split('CPU 0   ')[1].strip()
        s.add_cpu0(cpu0)
    elif 'CPU 1   ' in line:
        cpu1 = line.split('CPU 1   ')[1].strip()
        s.add_cpu1(cpu1)
    elif 'CPU 2   ' in line:
        cpu2 = line.split('CPU 2   ')[1].strip()
        s.add_cpu2(cpu2)
    elif 'CPU 3   ' in line:
        cpu3 = line.split('CPU 3   ')[1].strip()
        s.add_cpu3(cpu3)
    return s

def median(list):
    n = len(list)
    if n < 1:
            return 0
    if n % 2 == 1:
            return sorted(list)[n//2]
    else:
            return sum(sorted(list)[n//2-1:n//2+1])/2.0

def print_final_results(s):
    print('{ "data" : {')
    first = True
    for site, siteState in s.sites.items():
        if first:
            first = False
        else:
            print(',')
        print('"' + site + '" : ')
        print('{')
        print('  "site": "'  + site  + '",')

        print('  "cgmems": ' + str(siteState.cgmem) + ',')
        print('  "mems": '   + str(siteState.mem)   + ',')
        print('  "cpu0s": '  + str(siteState.cpu0)  + ',')
        print('  "cpu1s": '  + str(siteState.cpu1)  + ',')
        print('  "cpu2s": '  + str(siteState.cpu2)  + ',')
        print('  "cpu3s": '  + str(siteState.cpu3)  + ',')

        print('  "cgmem": ' + str(median(siteState.cgmem)) + ',')
        print('  "mem": '   + str(median(siteState.mem))   + ',')
        print('  "cpu0": '  + str(median(siteState.cpu0))  + ',')
        print('  "cpu1": '  + str(median(siteState.cpu1))  + ',')
        print('  "cpu2": '  + str(median(siteState.cpu2))  + ',')
        print('  "cpu3": '  + str(median(siteState.cpu3))       )

        print('}')
    print("} }")


def main():
    if len(sys.argv) < 2:
        print("Expected " + sys.argv[0] + " inputFileName")
        exit(1)
    inputFileName = sys.argv[1]
    s = PerfState()
    with open(inputFileName) as f:
        for line in f:
            s = handle_line(line, s)

    print_final_results(s)

main()