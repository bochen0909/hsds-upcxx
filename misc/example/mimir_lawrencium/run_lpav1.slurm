#!/bin/bash
#SBATCH --job-name=pglpav1
#SBATCH --partition=lr6
#SBATCH --qos=lr_normal
#SBATCH --account=pc_jgiga
#SBATCH --nodes=10
#SBATCH --ntasks-per-node=1
#SBATCH --time=03:00:00

start=`date +%s`

module unload openmpi
module load openmpi/2.0.2-gcc
module list


SCRATCH=/global/scratch/$USER
DATA=$SCRATCH/test/edge_mimir
OUTPUT=$SCRATCH/test/lpav1

ncpu=$(($SLURM_CPUS_ON_NODE-2))


rm -fr $OUTPUT && \
mkdir $OUTPUT && \
mpirun --mca routed radix pg_label_propagation_rand_long_short_v1  --ncpus $ncpu --graph $DATA --saveprefix $OUTPUT/output --engine_opts max_iterations=1000 --smin 187508

end=`date +%s`; runtime=$((end-start))
hours=$((runtime / 3600)); minutes=$(( (runtime % 3600) / 60 )); seconds=$(( (runtime % 3600) % 60 )); echo "Runtime: $hours:$minutes:$seconds (hh:mm:ss)"



