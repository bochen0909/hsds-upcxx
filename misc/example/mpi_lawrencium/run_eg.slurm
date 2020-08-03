#!/bin/bash
#SBATCH --job-name=edges_mpi
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
CURRDIR=`pwd`
DATA=$CURRDIR/krm
OUTPUT=$CURRDIR/edges

export SPARC_COMPRESS_MESSAGE=1

rm -fr $OUTPUT && \
mpirun edge_generating_mpi -i $DATA -o $OUTPUT  --min-shared-kmers 5 --max-degree 50 -z #--db leveldb -s /local/job"$SLURM_JOB_ID"

end=`date +%s`; runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



