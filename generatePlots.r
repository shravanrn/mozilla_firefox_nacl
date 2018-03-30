#!/usr/bin/env Rscript
# File looks like this
#   Build   mean    sd
#   Stock    12      4
#   DynLib   14      7
#   NaCl+C   16      9
#   NaCl+C++ 34     23

args = commandArgs(trailingOnly=TRUE)

if (length(args)==0) {
  stop("Expected input file.\n", call.=FALSE)
}

tests = list( "jpeg_render", "Final_JPEG_Time_core_inv", "Final_JPEG_Time_core_time", "Final_JPEG_Time_destroy", "Final_JPEG_Time_time", 
  "png_render", "Final_PNG_Time_core_inv", "Final_PNG_Time_core_time", "Final_PNG_Time_destroy", "Final_PNG_Time_time")

for(test in tests){
	renderTsv = paste(args[1], "_", test, ".tsv", sep="")
	renderSvg = paste(args[1], "_", test, ".svg", sep="")

	df = read.table(file = renderTsv, sep = '\t', header = TRUE)
	library(ggplot2)

	p = ggplot(df, aes(x=Build, y=mean, fill=Build)) +
		geom_bar(position=position_dodge(), stat="identity", colour='black') +
		geom_errorbar(aes(ymin=mean-sd, ymax=mean+sd), width=.2,position=position_dodge(.9)) +
		ggtitle(test)

	ggsave(renderSvg, p, "svg")
}

