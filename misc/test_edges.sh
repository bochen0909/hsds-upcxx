#!/bin/bash 


SCRIPTDIR=$(dirname $0)

SANDBOXDIR=$(pwd)

CAT1=cat
CAT2=cat

DATA=$SCRIPTDIR/../data/sample2
out1=${SANDBOXDIR}/test1
out2=${SANDBOXDIR}/test2

MPI_INPUT=${SANDBOXDIR}/krm_mpi
UPCXX_INPUT=${SANDBOXDIR}/krm_upcxx
MIMIR_INPUT=${SANDBOXDIR}/krm_mimir
MRMPI_INPUT=${SANDBOXDIR}/krm_mrmpi

compare_output() {

   NL=$($CAT1 $1/*|wc -l)
   if [ $NL -ne 44850 ] ; then 
   	 echo "number of line is not right:" $1 && exit -1
   	fi

   NL=$($CAT2 $2/*|wc -l)
   if [ $NL -ne 44850 ] ; then 
   	 echo "number of line is not right:" $2  && exit -1
 	fi

	out1=$1
	out2=$2
   
   	diff <($CAT1 $out1/* | sed 's/\t/ /g' | tr -s "[:blank:]"| sort) <($CAT2 $out2/* |sed 's/\t/ /g' | tr -s "[:blank:]"|  sort) > diff.log 2>&1
   	error=$? 
	return $error   	
}


get_input(){
	prog=$1
	if [[ $prog == edge_generating_mrmpi ]]; then
		echo ${MRMPI_INPUT}
	fi

	if [[ $prog == edge_generating_mimir ]]; then
		echo ${MIMIR_INPUT}
	fi
	
	if [[ $prog == edge_generating_mpi ]]; then
		echo ${MPI_INPUT}
	fi
	
	if [[ $prog == edge_generating_upc ]]; then
		echo ${UPCXX_INPUT}
	fi
	
}

test_generate_edges () {
	local prog1=$1
	local prog2=$2
	local opt1=$3
	local opt2=$4

	in1=$(get_input $prog1)	
	in2=$(get_input $prog2)

	MPICMD1=$MPICMD
	MPICMD2=$MPICMD
	if [[ $MPICMD1 == mpirun*  && $prog1 == *upc ]]; then
		MPICMD1="upcxx-run -n 4"
	fi
	if [[ $MPICMD2 == mpirun*  && $prog2 == *upc ]]; then
		MPICMD2="upcxx-run -n 4"
	fi
   CMD1="$PREFIX1 $MPICMD1 $prog1 $opt1"
   CMD2="$PREFIX2 $MPICMD2 $prog2 $opt2"
   echo Comparing \"$CMD1\" to \"$CMD2\"
   
   
   rm -fr $out1;rm -fr $out2
   
   if [ -z $COMPRESS1 ]; then
   		$CMD1 -i $in1 -o $out1 > log1.txt 2>&1 || (echo $CMD1 failed && tail log1.txt && exit -1)
   else
   		echo  prog1 uses compression.
   		SPARC_COMPRESS_MESSAGE=1 $CMD1 -i $in1 -o $out1 > log1.txt 2>&1 || (echo $CMD1 failed && tail log1.txt && exit -1)
   fi
   if [ -z $COMPRESS2 ]; then
        $CMD2 -i $in2 -o $out2  > log2.txt 2>&1 || (echo $CMD2 failed && tail log2.txt && exit -1)
   	else
   		echo prog2 uses compression.
   		SPARC_COMPRESS_MESSAGE=1 $CMD2 -i $in2 -o $out2  > log2.txt 2>&1 || (echo $CMD2 failed && tail log2.txt && exit -1)
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

rm -fr $MIMIR_INPUT && kmer_read_mapping_mimir -k 5 -i $DATA -o $MIMIR_INPUT >> /dev/null 2>&1
rm -fr $MRMPI_INPUT && kmer_read_mapping_mrmpi -k 5 -i $DATA -o $MRMPI_INPUT  >> /dev/null 2>&1
rm -fr $MPI_INPUT   && kmer_read_mapping_mpi -z -k 5 -i $DATA -o $MPI_INPUT  >> /dev/null 2>&1
rm -fr $UPCXX_INPUT   && kmer_read_mapping_upc -z -k 5 -i $DATA -o $UPCXX_INPUT  >> /dev/null 2>&1



for MPICMD in "" "mpirun -n 4"; do

	PARAM="--min-shared-kmers 0 --max-degree 10000000"
	PARAMZ="$PARAM -z"
	
	test_generate_edges edge_generating_upc edge_generating_mpi "$PARAM" "$PARAM"

	CAT1=zcat test_generate_edges edge_generating_upc edge_generating_upc "$PARAMZ" "$PARAM" 
		
	test_generate_edges edge_generating_mrmpi edge_generating_mimir "$PARAM" "$PARAM" 
	
	test_generate_edges edge_generating_mpi edge_generating_mimir  "$PARAM" "$PARAM"

	test_generate_edges edge_generating_mpi edge_generating_mpi "$PARAM -n 3"  "$PARAM" 
	
	CAT1=zcat test_generate_edges edge_generating_mpi edge_generating_mpi "$PARAMZ" "$PARAM" 
	
	COMPRESS1=1 test_generate_edges edge_generating_mpi edge_generating_mpi "$PARAM" "$PARAM" 
	
done 




