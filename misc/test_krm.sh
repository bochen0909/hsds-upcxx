#!/bin/bash 


SCRIPTDIR=$(dirname $0)

SANDBOXDIR=$(pwd)

CAT1=cat
CAT2=cat

sort_line(){
	echo $1 | tr -s " " | tr -s "\t" | tr " " "\n" | sort | tr "\n" " "  
}

compare_output() {
	out1=$1
	out2=$2
	if [ $($CAT1 $out1/* |wc -l) -ne $($CAT2 $out2/* | wc -l) ]; then
		echo number of line does not match
		return -1
	fi
	paste -d@ <($CAT1 $out1/* | sort -k 1) <($CAT2 $out2/* | sort -k 1) | while IFS="@" read -r ff1 ff2
	do
		f1=$(sort_line "$ff1")
		f2=$(sort_line "$ff2")
		
  		if [[ A"$f1" != A"$f2" ]];then
  			echo line not match
  			echo 1: $ff1
  			echo 2: $ff2
  			return -1
  		fi
  		
	done

	return 0
}

compare_output_python() {

   NL=$($CAT1 $1/*|wc -l)
   if [ $NL -ne 512 ] && [ $NL -ne 44959 ]; then 
   	 echo "number of line is not right" && exit -1
   	fi

   NL=$($CAT2 $2/*|wc -l)
   if [ $NL -ne 512 ] && [ $NL -ne 44959 ]; then 
   	 echo "number of line is not right" && exit -1
 	fi


	python $SCRIPTDIR/compare_krm_out.py <($CAT1 $1/*) <($CAT2 $2/*)
	return $?
}

test_krm () {
	local prog1=$1
	local prog2=$2
	local opt1=$3
	local opt2=$4
	
   local DATA=$SCRIPTDIR/../data/sample2

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
   
   compare_output_python $out1 $out2
   error=$? 
   if [ $error -eq 0 ];then
   	 echo "Test Passed."
   else
   		head diff.log
    	echo	"Test failed" && exit -1
    fi
   
}


for MPICMD in "" "mpirun -n 4"; do

	test_krm kmer_read_mapping_mpi kmer_read_mapping_upc "-k 25" "-k 25"
	
	CAT1=zcat test_krm kmer_read_mapping_upc kmer_read_mapping_upc "-k 5 -z" "-k 5"

	test_krm kmer_read_mapping_mrmpi kmer_read_mapping_mimir "-k 5" "-k 5"
	
	test_krm kmer_read_mapping_mrmpi kmer_read_mapping_mimir "-k 25" "-k 25"
	
	test_krm kmer_read_mapping_mpi kmer_read_mapping_mimir "-k 5" "-k 5"
	
	test_krm kmer_read_mapping_mpi kmer_read_mapping_mimir "-k 25" "-k 25"
	
	CAT1=zcat test_krm kmer_read_mapping_mpi kmer_read_mapping_mpi "-k 5 -z" "-k 5"
	
	COMPRESS1=1 test_krm kmer_read_mapping_mpi kmer_read_mapping_mpi "-k 5" "-k 5"

done 




