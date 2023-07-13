#!/bin/bash

#nprocesses=8

nthreads=8
# process nitems files for each thread.
nitems=8
nsteps=$(($nthreads * $nitems))
for i in `seq 0 $nsteps 1023`; do
	for t in `seq 0 $(($nthreads - 1))`; do
		offset=$((($t * $nthreads) + $i))
		
		python 01_normalize_mc4.py $offset $nitems &
		pids[$t]=$!
	done

	# wait for all processes
	for pid in ${pids[*]}; do
		wait $pid
	done

done 
