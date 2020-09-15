BASE=$1

fastq=~/scratch/biocrust/fastq_split/$BASE.fastq.gz
output=~/scratch/biocrust/fastq_lshvec/
LOCAL="/local/job""$SLURM_JOBID"
mkdir -p $output
mkdir -p  $LOCAL/$BASE 

ulimit -S -n 31200

python run_lshvec.py $fastq || exit -1


cd $LOCAL/$BASE && ls *.fastq | xargs -L1 -P20 gzip --fast && \
cd $LOCAL && tar cf $output/$BASE.tar $BASE  && rm -fr $BASE



