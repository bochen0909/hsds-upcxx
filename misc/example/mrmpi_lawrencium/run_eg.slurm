#!/bin/bash
#SBATCH --job-name=edges_mrmpi
#SBATCH --partition=lr6
#SBATCH --qos=lr_normal
#SBATCH --account=pc_jgiga
#SBATCH --nodes=10
#SBATCH --time=03:00:00

start=`date +%s`

module unload openmpi
module load openmpi/3.0.1-gcc
module list

SCRATCH=/global/scratch/$USER
DATA=$SCRATCH/test_mrmpi/krm_mrmpi
OUTPUT=$SCRATCH/test_mrmpi/edge_mrmpi

rm -fr $OUTPUT && \
mpirun edge_generating_mrmpi -i $DATA -o $OUTPUT  --min-shared-kmers 5 --max-degree 50

end=`date +%s`; runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



