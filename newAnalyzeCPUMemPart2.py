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

def computeSummary(outfile, sites, parsed1, parsed2, parsed3):
    with open(outfile, "w") as f:
        cgmems_overhead_nacl     = [ overhead(parsed2[site]["cgmem"], parsed1[site]["cgmem"]) for site in sites ]
        cgmems_overhead_ps       = [ overhead(parsed3[site]["cgmem"], parsed1[site]["cgmem"]) for site in sites ]
        # cgmems_overhead_ps_mutex = [ overhead(parsed4[site]["cgmem"], parsed1[site]["cgmem"]) for site in sites ]

        f.write("Peak Mem nacl Overhead: "     + str(average(cgmems_overhead_nacl)) + "\n")
        f.write("Peak Mem ps Overhead: "       + str(average(cgmems_overhead_ps)) + "\n")
        # # f.write("Peak Mem ps_mutex Overhead: " + str(average(cgmems_overhead_ps_mutex)) + "\n")

        mems_overhead_nacl     = [ overhead(parsed2[site]["mem"], parsed1[site]["mem"]) for site in sites ]
        mems_overhead_ps       = [ overhead(parsed3[site]["mem"], parsed1[site]["mem"]) for site in sites ]
        # mems_overhead_ps_mutex = [ overhead(parsed4[site]["mem"], parsed1[site]["mem"]) for site in sites ]

        f.write("Mem nacl Overhead: "     + str(average(mems_overhead_nacl)) + "\n")
        f.write("Mem ps Overhead: "       + str(average(mems_overhead_ps)) + "\n")
        # # f.write("Mem ps_mutex Overhead: " + str(average(mems_overhead_ps_mutex)) + "\n")

        primarycpu_overhead_nacl     = [ overhead(parsed2[site]["cpu1"], parsed1[site]["cpu1"]) for site in sites ]
        # primarycpu_overhead_ps_mutex = [ overhead(parsed4[site]["cpu3"], parsed1[site]["cpu1"]) for site in sites ]

        primarycpu_overhead_ps       = [ overhead(parsed3[site]["cpu1"], parsed1[site]["cpu1"]) for site in sites ]
        secondarycpu_overhead_ps     = [ additional_overhead(parsed3[site]["cpu3"], parsed1[site]["cpu1"]) for site in sites ]

        f.write("CPU nacl Overhead: "     + str(average(primarycpu_overhead_nacl)) + "\n")
        f.write("CPU ps Overhead: "       + str(average(primarycpu_overhead_ps)) + " + " + str(average(secondarycpu_overhead_ps)) + "\n")
        # f.write("CPU ps_mutex Overhead: " + str(average(primarycpu_overhead_ps_mutex)) + "\n")

def computeIndividual(outfile, sites, parsed1, parsed2, parsed3):
    with open(outfile, "w") as f:
        for site in sites:
            f.write("Site: " + site + "\n")
            cgmems_overhead_nacl     = overhead(parsed2[site]["cgmem"], parsed1[site]["cgmem"])
            cgmems_overhead_ps       = overhead(parsed3[site]["cgmem"], parsed1[site]["cgmem"])
            # cgmems_overhead_ps_mutex = overhead(parsed4[site]["cgmem"], parsed1[site]["cgmem"])

            f.write("Peak Mem nacl Overhead: "     + str(cgmems_overhead_nacl) + "\n")
            f.write("Peak Mem ps Overhead: "       + str(cgmems_overhead_ps) + "\n")
            # # f.write("Peak Mem ps_mutex Overhead: " + str(cgmems_overhead_ps_mutex) + "\n")

            mems_overhead_nacl     = overhead(parsed2[site]["mem"], parsed1[site]["mem"])
            mems_overhead_ps       = overhead(parsed3[site]["mem"], parsed1[site]["mem"])
            # mems_overhead_ps_mutex = overhead(parsed4[site]["mem"], parsed1[site]["mem"])

            f.write("Mem nacl Overhead: "     + str(mems_overhead_nacl) + "\n")
            f.write("Mem ps Overhead: "       + str(mems_overhead_ps) + "\n")
            # # f.write("Mem ps_mutex Overhead: " + str(average(mems_overhead_ps_mutex)) + "\n")

            primarycpu_overhead_nacl     = overhead(parsed2[site]["cpu1"], parsed1[site]["cpu1"])
            # primarycpu_overhead_ps_mutex = overhead(parsed4[site]["cpu3"], parsed1[site]["cpu1"])

            primarycpu_overhead_ps       = overhead(parsed3[site]["cpu1"], parsed1[site]["cpu1"])
            secondarycpu_overhead_ps     = additional_overhead(parsed3[site]["cpu3"], parsed1[site]["cpu1"])

            f.write("CPU nacl Overhead: "     + str(primarycpu_overhead_nacl) + "\n")
            f.write("CPU ps Overhead: "       + str(primarycpu_overhead_ps) + " + " + str(secondarycpu_overhead_ps) + "\n")
            # f.write("CPU ps_mutex Overhead: " + str(primarycpu_overhead_ps_mutex) + "\n")

            f.write("\n")

def main():
    if len(sys.argv) < 2:
        print("Expected " + sys.argv[0] + " inputFolderName")
        exit(1)
    inputFolderName = sys.argv[1]

    input1 = read(inputFolderName, "static_stock_cpu_mem_analysis.json")
    input2 = read(inputFolderName, "new_nacl_cpp_cpu_mem_analysis.json")
    input3 = read(inputFolderName, "new_ps_cpp_cpu_mem_analysis.json")
    # input4 = read(inputFolderName, "new_ps_cpp_mutex_cpu_mem_analysis.json")

    parsed1 = json.loads(input1)["data"]
    parsed2 = json.loads(input2)["data"]
    parsed3 = json.loads(input3)["data"]
    # parsed4 = json.loads(input4)["data"]

    sites = [parsed for parsed in parsed1]
    sites.remove("about:blank")

    summaryFile = os.path.join(inputFolderName, "analysis.txt")
    computeSummary(summaryFile, sites, parsed1, parsed2, parsed3)

    individualFile = os.path.join(inputFolderName, "analysis_individual.txt")
    computeIndividual(individualFile, sites, parsed1, parsed2, parsed3)


main()
