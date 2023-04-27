#!/bin/bash 

values=(1 8 16 24 32 40)

for value in "${values[@]}"
do
	echo "Processing '$value'_reader folder"
   cd "$value"_reader
   for counter in "${values[@]}"
   do
	grep "Writer exectime" result_"$counter"_"$value"_thread | awk -F' ' '{print $2}' | awk -F':' '{print $2}' > exectime_$counter
	grep "locktime" result_"$counter"_"$value"_thread | awk -F' ' '{print $2}' | awk -F':' '{print $2}' > locktime_$counter
   done
   cd ..
done

echo "Processing finished. Checkout the locktime_xx and exectime_xxx"
