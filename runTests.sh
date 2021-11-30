#!/bin/bash


# Exits program if any of the arguments is invalid.
validate_arguments(){

	if [ ! -d "$1" ]; then
		echo "ERROR: input directory doesn't exist."
		exit 1
	fi

	if [ ! -d "$2" ]; then
		echo "ERROR: output directory doesn't exist."
		exit 1
	fi

	if [ $3 -le 0 ]; then
		echo "ERROR: invalid maxThreads number."
		exit 1
	fi
}

############
### MAIN ###
############
	
#Initialize the variables
inputDir=$1
outputDir=$2
maxThreads=$3

validate_arguments ${inputDir} ${outputDir} ${maxThreads}
	
for input in $(ls ${inputDir}/*.txt); do

	#Run program with every input from 1 thread to maxThreads in that order
	for i in $(seq 1 ${maxThreads}); do

		#filteredInput is the name of the input without parent directory or extension
		filteredInput="$(basename -- $input)";
		filteredInput="$(echo $filteredInput | cut -d '.' -f 1)"
		
		#outputFile is the path of the output file
		outputFile=${filteredInput}-${i}.txt	
		outputFile=${outputDir}/${outputFile}

		echo InputFile=${input} NumThreads=${i}

		./tecnicofs ${input} ${outputFile} ${i} | grep "TecnicoFS" #Run the program only outputting the last line

	done

done
