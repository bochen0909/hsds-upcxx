#!/bin/bash
#SBATCH --job-name=kc_mrmpi
#SBATCH --partition=lr6
#SBATCH --qos=lr_normal
#SBATCH --account=pc_jgiga
#SBATCH --nodes=5
#SBATCH --time=02:00:00

start=`date +%s`

module unload openmpi
module load openmpi/3.0.1-gcc
module list

SCRATCH=/global/scratch/$USER
DATA=$SCRATCH/test_mrmpi/seq
OUTPUT=$SCRATCH/test_mrmpi/kc_mimir

rm -fr $OUTPUT && \
mpirun kmer_counting_mrmpi -k 17 -i $DATA -o $OUTPUT

end=`date +%s`
runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



