#!/bin/bash
#SBATCH --job-name=krm_mimir
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
DATA=$SCRATCH/test/seq
OUTPUT=$SCRATCH/test/krm_mimir

export MIMIR_MAX_RECORD_SIZE=2M

rm -fr $OUTPUT && \
mpirun kmer_read_mapping_mimir -k 17 -i $DATA -o $OUTPUT

end=`date +%s`
runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



