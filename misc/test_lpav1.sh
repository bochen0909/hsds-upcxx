#!/bin/bash 


SCRIPTDIR=$(dirname $0)

SANDBOXDIR=$(pwd)

CAT1=cat
CAT2=cat

DATA=$SCRIPTDIR/../data/sample2
out1=${SANDBOXDIR}/test1
out2=${SANDBOXDIR}/test2

MPI_KRM_INPUT=${SANDBOXDIR}/krm_mpi
MPI_EDGE_INPUT=${SANDBOXDIR}/edge_mpi

compare_output() {

   NL=$($CAT1 $1/*|wc -l)
   if [ $NL -ne 300 ] ; then 
   	 echo "number of line is not right:" $1 && exit -1
   	fi

   NL=$($CAT2 $2/*|wc -l)
   if [ $NL -ne 300 ] ; then 
   	 echo "number of line is not right:" $2  && exit -1
 	fi

	out1=$1
	out2=$2
   
   	diff <($CAT1 $out1/* | sed 's/\t/ /g' | tr -s "[:blank:]"| sort) <($CAT2 $out2/* |sed 's/\t/ /g' | tr -s "[:blank:]"|  sort) > diff.log 2>&1
   	error=$? 
	return $error   	
}


test_lpav1 () {
	local prog1=$1
	local prog2=$2
	local opt1=$3
	local opt2=$4

	in1=$MPI_EDGE_INPUT	
	in2=$in1

   CMD1="$PREFIX1 $MPICMD $prog1 $opt1"
   CMD2="$PREFIX2 $MPICMD $prog2 $opt2"
   echo Comparing \"$CMD1\" to \"$CMD2\"
   
   
   rm -fr $out1;rm -fr $out2
   
   if [ -z $COMPRESS1 ]; then
   		$CMD1 -o $out1 $in1 > log1.txt 2>&1 || (echo $CMD1 failed && tail log1.txt && exit -1)
   else
   		echo  prog1 uses compression.
   		SPARC_COMPRESS_MESSAGE=1 $CMD1 -o $out1 $in1 > log1.txt 2>&1 || (echo $CMD1 failed && tail log1.txt && exit -1)
   fi
   if [ -z $COMPRESS2 ]; then
        $CMD2 -o $out2 $in2 > log2.txt 2>&1 || (echo $CMD2 failed && tail log2.txt && exit -1)
   	else
   		echo prog2 uses compression.
   		SPARC_COMPRESS_MESSAGE=1 $CMD2 -o $out2 $in2  > log2.txt 2>&1 || (echo $CMD2 failed && tail log2.txt && exit -1)
   	fi
   
   compare_output $out1 $out2
   error=$? 
   if [ $error -eq 0 ];then
   	 echo "Test Passed."
   else
   		head diff.log
    	echo	"Test failed" && exit -1
    fi
   
}

rm -fr $MPI_KRM_INPUT   && kmer_read_mapping_mpi -z -k 5 -i $DATA -o $MPI_KRM_INPUT  >> /dev/null 2>&1
rm -fr $MPI_EDGE_INPUT   && edge_generating_mpi -i $MPI_KRM_INPUT -o $MPI_EDGE_INPUT -z --min-shared-kmers 0 --max-degree 10000000  >> /dev/null 2>&1



for MPICMD in "" "mpirun -n 4"; do

	PARAM="-n 1000"
	PARAMZ="$PARAM -z"
	CAT2=zcat test_lpav1 lpav1_mpi lpav1_mpi "$PARAM" "$PARAMZ" 
	
	COMPRESS1=1 test_lpav1 lpav1_mpi lpav1_mpi "$PARAM" "$PARAM"
	
	test_lpav1 lpav1_mpi lpav1_upc "$PARAM" "$PARAM"
	
	CAT2=zcat test_lpav1 lpav1_upc lpav1_upc "$PARAM" "$PARAMZ"  

done 




