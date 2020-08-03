#!/bin/bash
#SBATCH --job-name=krm_upcxx
#SBATCH --partition=lr4
#SBATCH --qos=lr_normal
#SBATCH --account=pc_jgiga
#SBATCH --nodes=15
#SBATCH --cpus-per-task=1
#SBATCH --time=02:00:00

start=`date +%s`

module unload openmpi
module load openmpi/3.0.1-gcc
module list

SCRATCH=/global/scratch/$USER
CURRDIR=`pwd`
DATA=$CURRDIR/seq
OUTPUT=$CURRDIR/krm

export GASNET_PHYSMEM_MAX='30 GB'

CMD=$HOME/mydev/sparc-mpi/build/kmer_read_mapping_upc

CPUPERNODE=`echo $SLURM_JOB_CPUS_PER_NODE | cut -f 1 -d \(`
CPUPERNODE=$(($CPUPERNODE - 1))
NJOBS=$(($CPUPERNODE * $SLURM_JOB_NUM_NODES))

echo "Using upcxx-run -n $NJOBS -N $SLURM_JOB_NUM_NODES " 

rm -fr $OUTPUT && \
upcxx-run -n $NJOBS -N $SLURM_JOB_NUM_NODES  $CMD -k 17 -i $DATA -o $OUTPUT -z 

end=`date +%s`
runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



