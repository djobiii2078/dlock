#!/bin/bash 

values=(1 8 16 24 32 40)
writers=(8 16 24 32 40)

for value in "${values[@]}"
do
	echo "Processing atomic '$value'_reader folder"
	mkdir -p atomic/"$value"_reader
     	cd atomic/"$value"_reader
   for counter in "${writers[@]}"
   do
   	../../../lock.o $counter $value .01 > result_"$counter"_"$value"_thread 
   done
   cd ../..
done

echo "Processing atomic finished. Checkout the locktime_xx and exectime_xxx"

