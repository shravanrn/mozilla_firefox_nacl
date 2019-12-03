#!/usr/bin/env python3

import math
import sys
import csv
import os
from urllib.parse import urlparse
import simplejson as json

def overhead(val1, val2):
    return 100.0 * (val1 - val2) / val2

def additional_overhead(val1, val2):
    return 100.0 * val1 / val2

def average(list):
    n = len(list)
    if n < 1:
            return 0
    return float(sum(list))/n

def read(folder, filename):
    inputFileName1 = os.path.join(folder, filename)
    with open(inputFileName1) as f:
        input1 = f.read()
    return input1

def main():
    if len(sys.argv) < 2:
        print("Expected " + sys.argv[0] + " inputFolderName")
        exit(1)
    inputFolderName = sys.argv[1]

    input1 = read(inputFolderName, "static_stock_cpu_mem_analysis.json")
    input2 = read(inputFolderName, "new_nacl_cpp_cpu_mem_analysis.json")
    input3 = read(inputFolderName, "new_ps_cpp_cpu_mem_analysis.json")
    input4 = read(inputFolderName, "new_ps_cpp_mutex_cpu_mem_analysis.json")

    parsed1 = json.loads(input1)["data"]
    parsed2 = json.loads(input2)["data"]
    parsed3 = json.loads(input3)["data"]
    parsed4 = json.loads(input4)["data"]

    sites = [parsed for parsed in parsed1]
    sites.remove("about:blank")

    cgmems_overhead_nacl     = [ overhead(parsed2[site]["cgmem"], parsed1[site]["cgmem"]) for site in sites ]
    cgmems_overhead_ps       = [ overhead(parsed3[site]["cgmem"], parsed1[site]["cgmem"]) for site in sites ]
    cgmems_overhead_ps_mutex = [ overhead(parsed4[site]["cgmem"], parsed1[site]["cgmem"]) for site in sites ]

    print("Mem nacl Overhead: "     + str(average(cgmems_overhead_nacl)))
    print("Mem ps Overhead: "       + str(average(cgmems_overhead_ps)))
    print("Mem ps_mutex Overhead: " + str(average(cgmems_overhead_ps_mutex)))

    primarycpu_overhead_nacl     = [ overhead(parsed2[site]["cpu1"], parsed1[site]["cpu1"]) for site in sites ]
    primarycpu_overhead_ps_mutex = [ overhead(parsed4[site]["cpu3"], parsed1[site]["cpu1"]) for site in sites ]

    primarycpu_overhead_ps       = [ overhead(parsed3[site]["cpu1"], parsed1[site]["cpu1"]) for site in sites ]
    secondarycpu_overhead_ps     = [ additional_overhead(parsed3[site]["cpu3"], parsed1[site]["cpu1"]) for site in sites ]

    print("CPU nacl Overhead: "     + str(average(primarycpu_overhead_nacl)))
    print("CPU ps Overhead: "       + str(average(primarycpu_overhead_ps)) + " + " + str(average(secondarycpu_overhead_ps)))
    print("CPU ps_mutex Overhead: " + str(average(primarycpu_overhead_ps_mutex)))


main()