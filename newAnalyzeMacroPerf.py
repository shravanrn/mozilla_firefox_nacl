#!/usr/bin/env python3

import os
import os.path
import sys
import simplejson as json
from statistics import mean, median, stdev

def read(folder, filename):
    inputFileName1 = os.path.join(folder, filename)
    if not os.path.isfile(inputFileName1):
        return None
    with open(inputFileName1) as f:
        input1 = f.read()
    return input1

def readJson(folder, filename):
    data = read(folder, filename)
    return json.loads(data)

def readJsonTestValue(folder, filename):
    input = readJson(folder, filename)
    ret = input["suites"][0]["subtests"][0]["value"]
    return ret

def writeJson(folder, filename, obj):
    str = json.dumps(obj, indent=4)
    full_file = os.path.join(folder, filename)
    with open(full_file, "w") as text_file:
        text_file.write("%s\n" % str)

def writeMetrics(fileobj, label, arr):
    fileobj.write("Mean {}: {}\n".format(label, mean(arr)))
    fileobj.write("Median {}: {}\n".format(label, median(arr)))
    fileobj.write("Std_Dev {}: {}\n".format(label, stdev(arr)))

def getMedianTestValue(inputFolderName, fileTemplate):
    vals = []
    for i in range(1, 11):
        vals.append(readJsonTestValue(inputFolderName, fileTemplate.format(str(i))))
    return median(vals)

def getMedianIntValue(inputFolderName, fileTemplate):
    vals = []
    for i in range(1, 11):
        contents = read(inputFolderName, fileTemplate.format(str(i)))
        if contents:
            vals.append(int(contents))
    return median(vals)

def main():
    if len(sys.argv) < 2:
        print("Expected " + sys.argv[0] + " inputFolderName")
        exit(1)
    inputFolderName = sys.argv[1]

    builds = {
        "stock" : "static_stock_external_page_render{}",
        "sfi" : "new_nacl_cpp_external_page_render{}",
        "psspin" : "new_ps_cpp_external_page_render{}"
    }

    other_builds = [build for build in builds.keys()]
    other_builds.remove("stock")

    sites = [
        "google.com",
        "yelp.com",
        "eurosport.com",
        "legacy.com",
        "reddit.com",
        "seatguru.com",
        "twitch.tv",
        "amazon.com",
        "economist.com",
        "espn.com",
        "wowprogress.com"
    ]

    latency_output = {}
    for build, build_output in builds.items():
        latency_output[build] = []
        for i in range(1, len(sites)):
            siteobj = {
                "name" : sites[i],
                "value" : getMedianTestValue(inputFolderName, build_output.format(str(i) + "-{}.json"))
            }
            latency_output[build].append(siteobj)

    writeJson(inputFolderName, "page_latency.json", latency_output)

    with open(os.path.join(inputFolderName, "page_latency_metrics.txt"), "w") as text_file:
        for build in other_builds:
            overheads = []
            for i in range(1, len(sites)):
                stock_val = getMedianTestValue(inputFolderName, builds["stock"].format(str(i) + "-{}.json"))
                build_val = getMedianTestValue(inputFolderName, builds[build].format(str(i) + "-{}.json"))
                overhead = 100.0 * ((build_val/stock_val) - 1.0)
                overheads.append(overhead)
            writeMetrics(text_file, "{} latency".format(build), overheads)

    memory_output = {}
    for build, build_output in builds.items():
        memory_output[build] = []
        for i in range(1, len(sites)):
            siteobj = {
                "name" : sites[i],
                "value" : getMedianIntValue(inputFolderName, build_output.format(str(i) + "_mem-{}.txt"))
            }
            memory_output[build].append(siteobj)

    writeJson(inputFolderName, "page_memory_overhead.json", memory_output)

    with open(os.path.join(inputFolderName, "page_memory_overhead_metrics.txt"), "w") as text_file:
        for build in other_builds:
            overheads = []
            for i in range(1, len(sites)):
                stock_val = getMedianIntValue(inputFolderName, builds["stock"].format(str(i) + "_mem-{}.txt"))
                build_val = getMedianIntValue(inputFolderName, builds[build].format(str(i) + "_mem-{}.txt"))
                overhead = 100.0 * ((build_val/stock_val) - 1.0)
                overheads.append(overhead)
            writeMetrics(text_file, "{} memory overhead".format(build), overheads)

main()
