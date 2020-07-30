#!/bin/bash 


SCRIPTDIR=$(dirname $0)

SANDBOXDIR=$(pwd)

CAT1=cat
CAT2=cat

test_kmer_counting () {
	local prog1=$1
	local prog2=$2
	local opt1=$3
	local opt2=$4
	
   local DATA=$SCRIPTDIR/../data/sample2
   CMD1="$PREFIX1 $MPICMD $prog1 $opt1"
   CMD2="$PREFIX2 $MPICMD $prog2 $opt2"
   echo Comparing \"$CMD1\" to \"$CMD2\"
   
   local out1=${SANDBOXDIR}/test1
   local out2=${SANDBOXDIR}/test2
   
   rm -fr $out1;rm -fr $out2
   
   if [ -z $COMPRESS1 ]; then
   		$CMD1 -i $DATA -o $out1 > log1.txt 2>&1 || (echo $CMD1 failed && tail log1.txt && exit -1)
   else
   		echo  prog1 uses compression.
   		SPARC_COMPRESS_MESSAGE=1 $CMD1 -i $DATA -o $out1 > log1.txt 2>&1 || (echo $CMD1 failed && tail log1.txt && exit -1)
   fi
   if [ -z $COMPRESS2 ]; then
        $CMD2 -i $DATA -o $out2  > log2.txt 2>&1 || (echo $CMD2 failed && tail log2.txt && exit -1)
   	else
   		echo prog2 uses compression.
   		SPARC_COMPRESS_MESSAGE=1 $CMD2 -i $DATA -o $out2  > log2.txt 2>&1 || (echo $CMD2 failed && tail log2.txt && exit -1)
   	fi

   NL=$($CAT1 $out1/*|wc -l)
   if [ $NL -ne 512 ] && [ $NL -ne 44959 ]; then 
   	 echo "number of line is not right" && exit -1
   	fi

   NL=$($CAT2 $out2/*|wc -l)
   if [ $NL -ne 512 ] && [ $NL -ne 44959 ]; then 
   	 echo "number of line is not right" && exit -1
 	fi

   
   diff <($CAT1 $out1/* |sed 's/\t/ /g' | sort) <($CAT2 $out2/* |sed 's/\t/ /g' | sort) > diff.log 2>&1
   error=$? 
   if [ $error -eq 0 ];then
   	 echo "Test Passed."
   else
   		head diff.log
    	echo	"Test failed" && exit -1
    fi
   
}


for MPICMD in "" "mpirun -n 4"; do

	test_kmer_counting kmer_counting_mrmpi kmer_counting_mimir "-k 5" "-k 5"
	test_kmer_counting kmer_counting_mrmpi kmer_counting_mimir "-k 25" "-k 25"
	
	test_kmer_counting kmer_counting_mpi kmer_counting_mimir "-k 5" "-k 5"
	test_kmer_counting kmer_counting_mpi kmer_counting_mimir "-k 25" "-k 25"
	
	CAT1=zcat test_kmer_counting kmer_counting_mpi kmer_counting_mpi "-k 5 -z" "-k 5"
	
	COMPRESS1=1 test_kmer_counting kmer_counting_mpi kmer_counting_mpi "-k 5" "-k 5"
	
	test_kmer_counting kmer_counting_mpi kmer_counting_upc "-k 25" "-k 25"
	
	CAT1=zcat test_kmer_counting kmer_counting_upc kmer_counting_upc "-k 5 -z" "-k 5"
	
done 




