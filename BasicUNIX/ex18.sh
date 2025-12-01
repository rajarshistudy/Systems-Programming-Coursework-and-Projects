#!/bin/bash

# Filename: ex18.sh
#
# Problem: Find the highest number for each line and print it. 
cat ex18.input | awk '{
	max=$1
	for (i=2; i<=NF; i++) {
		if ($i > max) max = $i
	}
	print max
	}'
