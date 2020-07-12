# Example using MR-MPI map reduce 

Sample run of 7G fasta sequences (3.5G nanopore and 3.5G illumina) on LBL LAWRENCIUM cluster.
Each node has 32 cores and 192G memory. MR-MPI utilized little memory while spilling big pages to disk.   


|                     | #node | runtime |
|---------------------|-------|---------|
| K-mer counting      | 5     | 11 min  |
| K-mer reads mapping | 5     | 14 min  |
| edge generating     | 10    | 52 min  |

