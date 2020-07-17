#!/bin/bash 


SCRIPTDIR=$(dirname $0)

SANDBOXDIR=$(pwd)

CAT1=cat
CAT2=cat

DATA=$SCRIPTDIR/../data/sample2
out1=${SANDBOXDIR}/test1
out2=${SANDBOXDIR}/test2
out3=${SANDBOXDIR}/test2.merge

MPI_INPUT=${SANDBOXDIR}/krm_mpi
MIMIR_INPUT=${SANDBOXDIR}/krm_mimir
MRMPI_INPUT=${SANDBOXDIR}/krm_mrmpi

compare_output() {

   NL=$($CAT1 $out1/*|wc -l)
   if [ $NL -ne 44850 ] ; then 
   	 echo "number of line is not right:" $1 && exit -1
   	fi

   NL=$($CAT2 $out3/*|wc -l)
   if [ $NL -ne 44850 ] ; then 
   	 echo "number of line is not right:" $2  && exit -1
 	fi

   
   	diff <($CAT1 $out1/* | sed 's/\t/ /g' | tr -s "[:blank:]"| sort) <($CAT2 $out3/* |sed 's/\t/ /g' | tr -s "[:blank:]"|  sort) > diff.log 2>&1
   	error=$? 
	return $error   	
}


get_input(){
	prog=$1
	if [[ $prog == mrmpi ]]; then
		echo ${MRMPI_INPUT}
	fi

	if [[ $prog == mimir ]]; then
		echo ${MIMIR_INPUT}
	fi
	
	if [[ $prog == mpi ]]; then
		echo ${MPI_INPUT}
	fi
	
}

test_merge () {

	local prog1=edge_generating_"$1"
	local prog2=$prog1
	local opt1=$2
	local opt2=$3

	in1=$(get_input $1)	
	in2="$in1".split

   CMD1="$PREFIX1 $MPICMD $prog1 $opt1"
   CMD2="$PREFIX2 $MPICMD $prog2 $opt2"
   echo Comparing \"$CMD1\" to \"$CMD2\"
   
   
   rm -fr $out1;rm -fr $out2 && mkdir $out2; rm -fr $out3
   
   $CMD1 -i $in1 -o $out1 > log1.txt 2>&1 || (echo $CMD1 failed && tail log1.txt && exit -1)
   
   declare -a inputs
   inputs=()
	for a in "${names[@]}"; do
		splitinput="$in2"/"$a".d
		splitoutput="$out2"/"$a".d
		inputs+=("$splitoutput")
		$CMD2 -i $splitinput -o $splitoutput > log2.txt 2>&1 || (echo $CMD2 failed && tail log2.txt && exit -1)
	done   
   

  if [[ $1 == mrmpi ]]; then
  	merge_intrmed_mpi -n $NBUCKET -o $out3 --sep "\t" --sp 2 "${inputs[@]}"  > log3.txt 2>&1
  else
  	merge_intrmed_mpi -n $NBUCKET -o $out3 --sep " " --sp 2 "${inputs[@]}" > log3.txt 2>&1
  fi   
    

    
   
   compare_output
   error=$? 
   if [ $error -eq 0 ];then
   	 echo "Test Passed."
   else
   		head diff.log
    	echo	"Test failed" && exit -1
    fi
   
}

rm -fr $MIMIR_INPUT && kmer_read_mapping_mimir -k 5 -i $DATA -o $MIMIR_INPUT >> /dev/null 2>&1
rm -fr $MRMPI_INPUT && kmer_read_mapping_mrmpi -k 5 -i $DATA -o $MRMPI_INPUT  >> /dev/null 2>&1
rm -fr $MPI_INPUT   && kmer_read_mapping_mpi  -k 5 -i $DATA -o $MPI_INPUT  >> /dev/null 2>&1

declare -a names
names=( xaa  xab  xac  xad  xae )

for oldfolder in $MIMIR_INPUT $MRMPI_INPUT $MPI_INPUT; do
	OLDPWD=`pwd`
	folder="$oldfolder".split
	rm -fr $folder && cp -r $oldfolder $folder
	cd $folder && split -n l/5 * 
	for a in "${names[@]}"; do
		mkdir "$a".d && mv $a "$a".d/ 
	done   
	cd $OLDPWD
done 

for MPICMD in "" "mpirun -n 4"; do
	for NBUCKET in 1 3; do
		PARAM="--min-shared-kmers 0 --max-degree 10000000"
		PARAMZ="$PARAM -z"

		test_merge mrmpi  "$PARAM" "$PARAM"
		test_merge mimir "$PARAM" "$PARAM"
		CAT1=zcat test_merge mpi "$PARAMZ" "$PARAM"		 
 
	done
done 




