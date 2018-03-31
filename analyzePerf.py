#!/usr/bin/env python3

# Should work in python 2 or 3
import math
import sys

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
		self.currentBuildName = ""
		self.currentTestName = ""
		self.currentValueName = ""
		self.timings = {}


def addTestValue(s):
	if(s.currentTestName == ""):
		return
	if s.currentTestName not in s.timings:
		s.timings[s.currentTestName] = { }
	if s.currentBuildName not in s.timings[s.currentTestName]:
		s.timings[s.currentTestName][s.currentBuildName] = []	
	s.timings[s.currentTestName][s.currentBuildName].append(float(s.currentValueName))

def handle_line(line, s):
	if "INFO -" in line:
		return s

	if line.startswith("FFBuild-"):
		s.currentBuildName = line.split("FFBuild-")[1].split("\n")[0]
	elif "\"name\"" in line:
		s.currentTestName = line.split("\": \"")[1].split("\",")[0].split(".html")[0]
	elif "\"value\"" in line:
		s.currentValueName = float(line.split("\": ")[1].split("\n")[0])
		addTestValue(s)
		s.currentTestName = ""
	elif "Final JPEG_Time" in line:
		s.currentTestName = "Final_JPEG_Time_time"
		s.currentValueName = float(line.split(" time=")[1].split(" ")[0].replace(',', '')) / 1000000.0
		addTestValue(s)
		s.currentTestName = "Final_JPEG_Time_core_inv"
		s.currentValueName = float(line.split(" core_inv=")[1].split(" ")[0].replace(',', ''))
		addTestValue(s)
		s.currentTestName = "Final_JPEG_Time_core_time"
		s.currentValueName = float(line.split(" core_time=")[1].split(" ")[0].replace(',', '')) / 1000000.0
		addTestValue(s)
		s.currentTestName = "Final_JPEG_Time_destroy"
		s.currentValueName = float(line.split(" destroy=")[1].split(" ")[0].replace(',', '')) / 1000000.0
		addTestValue(s)
	elif "Final PNG_Time" in line:
		s.currentTestName = "Final_PNG_Time_time"
		s.currentValueName = float(line.split(" time=")[1].split(" ")[0].replace(',', '')) / 1000000.0
		addTestValue(s)
		s.currentTestName = "Final_PNG_Time_core_inv"
		s.currentValueName = float(line.split(" core_inv=")[1].split(" ")[0].replace(',', ''))
		addTestValue(s)
		s.currentTestName = "Final_PNG_Time_core_time"
		s.currentValueName = float(line.split(" core_time=")[1].split(" ")[0].replace(',', '')) / 1000000.0
		addTestValue(s)
		s.currentTestName = "Final_PNG_Time_destroy"
		s.currentValueName = float(line.split(" destroy=")[1].split(" ")[0].replace(',', '')) / 1000000.0
		addTestValue(s)

	return s

def average(list):
	return float(sum(list))/len(list)

def print_final_results(s, opFile):
	for test, buildObj in s.timings.items():
		opText = "Build\tmean\tsd\n"
		for build, timeList in buildObj.items():
			avg = average(timeList)
			variance = list(map(lambda x: (x - avg)**2, timeList))
			stdDev = math.sqrt(average(variance))
			opText += build + "\t" +  str("{0:.2f}".format(avg)) + "\t" + str("{0:.2f}".format(stdDev)) + "\n"
		with open(opFile + "_" + test + ".tsv", "w") as text_file:
			text_file.write("%s" % opText)

def main():
	if len(sys.argv) < 3:
		print("Expected " + sys.argv[0] + " inputFileName outputPath/Prefix")
		exit(1)
	s = PerfState()
	with open(sys.argv[1]) as f:
		for line in f:
			s = handle_line(line, s)
	print_final_results(s, sys.argv[2])

main()
