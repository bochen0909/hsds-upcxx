#!/bin/bash
#SBATCH --job-name=kc_mimir
#SBATCH --partition=lr5
#SBATCH --qos=lr_normal
#SBATCH --account=pc_jgiga
#SBATCH --nodes=5
#SBATCH --cpus-per-task=1
#SBATCH --time=02:00:00

start=`date +%s`

module unload openmpi
#module load openmpi/3.0.1-gcc
module load openmpi/3.1.4-gcc
module list

SCRATCH=/global/scratch/$USER
CURRDIR=`pwd`
DATA=$CURRDIR/seq
OUTPUT=$CURRDIR/kc

rm -fr $OUTPUT && \
mpirun kmer_counting_mpi -k 17 -i $DATA -o $OUTPUT -z #--db leveldb --scratch /dev/shm

end=`date +%s`
runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



