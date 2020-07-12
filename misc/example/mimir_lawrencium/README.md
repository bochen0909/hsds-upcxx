# Example using Mimir map reduce 

A sample run of 7G fasta sequences (3.5G nanopore and 3.5G illumina) on LBL LAWRENCIUM cluster.
Each node has 32 cores and 192G memory. All running are CPU Bound.  


|                     | #node | runtime |
|---------------------|-------|---------|
| K-mer counting      | 5     | 13 min  |
| K-mer reads mapping | 5     | 12 min  |
| edge generating     | 10    | 22 min  |
| LPA                 | 10    | 51 min  |

