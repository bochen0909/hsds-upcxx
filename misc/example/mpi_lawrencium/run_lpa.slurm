#!/bin/bash
#SBATCH --job-name=lpav1_mpi
#SBATCH --partition=lr4
#SBATCH --qos=lr_normal
#SBATCH --account=pc_jgiga
#SBATCH --nodes=5
#SBATCH --time=1:00:00

start=`date +%s`

module unload openmpi
module load openmpi/3.0.1-gcc
module list

SCRATCH=/global/scratch/$USER
CURRDIR=`pwd`
DATA=$CURRDIR/edges
OUTPUT=$CURRDIR/clu

export SPARC_COMPRESS_MESSAGE=1

rm -fr $OUTPUT && \
mpirun lpav1_mpi -o $OUTPUT  --smin 187508 -n 500 $DATA -w


end=`date +%s`; runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



