#!/bin/bash
#SBATCH --job-name=lpa_upcxx
#SBATCH --partition=lr4
#SBATCH --qos=lr_normal
#SBATCH --account=pc_jgiga
#SBATCH --nodes=25
#SBATCH --cpus-per-task=1
#SBATCH --time=02:00:00

start=`date +%s`

module unload openmpi
module load openmpi/3.0.1-gcc
module list

SCRATCH=/global/scratch/$USER
CURRDIR=`pwd`
DATA=$CURRDIR/edges
OUTPUT=$CURRDIR/clu

export GASNET_PHYSMEM_MAX='30 GB'

CMD=$HOME/mydev/sparc-mpi/build/lpav1_upc
#CMD=$HOME/mydev/sparc-mpi/Debug/lpav1_upc
export  GASNET_BACKTRACE=1

CPUPERNODE=`echo $SLURM_JOB_CPUS_PER_NODE | cut -f 1 -d \(`
CPUPERNODE=$(($CPUPERNODE - 1))
NJOBS=$(($CPUPERNODE * $SLURM_JOB_NUM_NODES))

echo "Using upcxx-run -n $NJOBS -N $SLURM_JOB_NUM_NODES " 

rm -fr $OUTPUT && \
upcxx-run -n $NJOBS -N $SLURM_JOB_NUM_NODES  $CMD   -o $OUTPUT  --smin 187508 -n 500 $DATA -w

end=`date +%s`
runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



