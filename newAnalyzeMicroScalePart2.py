#!/usr/bin/env python3

import math
import sys
import csv
import os
from urllib.parse import urlparse
import simplejson as json

def getMedian(els, group, image):
    for el in els:
        if group in el["Group"] and image in el["Group"]:
            return float(el["Median"].replace(',', ''))
    sys.exit("Unreachable")

def getScale(el):
    group = el["Group"]
    u = urlparse(group)
    totalStr = u.netloc.split('.', 1)[0]
    total = totalStr.split("total", 1)[1]
    return int(total)

def computeSummary(summaryFile, parsed1, parsed2, parsed3):
    scales_dups = [getScale(el) for el in parsed1]
    scales = list(set(scales_dups))
    scales.sort()
    with open(summaryFile, "w") as f:
        writer = csv.writer(f)
        for scale in scales:
            row = [ scale ]
            group = "total" + str(scale)
            for image in ["_large.jpeg", "_small.jpeg"]:
                stock_val = getMedian(parsed1, group, image)
                nacl_val = getMedian(parsed2, group, image)
                ps_val = getMedian(parsed3, group, image)
                row.append(nacl_val/stock_val)
                row.append(ps_val/stock_val)
            writer.writerow(row)

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

    input1 = read(inputFolderName, "static_stock_terminal_analysis.json")
    input2 = read(inputFolderName, "new_nacl_cpp_terminal_analysis.json")
    input3 = read(inputFolderName, "new_ps_cpp_terminal_analysis.json")

    parsed1 = json.loads(input1)["data"]
    parsed2 = json.loads(input2)["data"]
    parsed3 = json.loads(input3)["data"]

    computeSummary(os.path.join(inputFolderName, "sandbox_scaling.dat"), parsed1, parsed2, parsed3)

main()
