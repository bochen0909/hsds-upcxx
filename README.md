# Hierarchical Clustering of DNA sequence 
# (working in progress......)

**Sparc_upcxx** is a re-implementation that uses [UPC++](https://bitbucket.org/berkeleylab/upcxx/wiki/Home). It performs similar or even better than sparc_mpi. However I find it might be tricky to run a upcxx program since the data transfer is hidden from users. I found failure sometimes when lots of rpc calls were performed asynchronously. 

When running the programs, make sure there are enough nodes to hold all the data in memory. Although some programs support storing temporary data in disk, but it will make the progress really slow.

### Example Runs

Please find sbatch scripts of sample runs on [LAWRENCIUM](https://sites.google.com/a/lbl.gov/high-performance-computing-services-group/lbnl-supercluster/lawrencium) in [misc/example](misc/example) folder.



## Installation



First clone the code

```
    git clone https://github.com/Lizhen0909/sparc-mpi.git
    cd sparc-mpi && git submodule update --init --recursive
```

All the versions are independent from each other, you may choose to only build the version that you are interested in.



## Usages

Similar to [Sparc](https://bitbucket.org/LizhenShi/sparc/src/master/README.md),  given a sequence file the flow of analysis includes 4 steps. For each program use "-h" or "--help" option to get help.

### Kmer Counting (optional)

Find the kmer counting profile of the data, so that we decides how to filter out "bad" kmers. In this step we run edge_generating_$SURFIX where $SURFIX means mrmpi, mimir, mpi or upcxx. 

For example for mpi version:
```
    $./kmer_counting_mpi -h
    
    -h, --help
    shows this help message
    -i, --input
    input folder which contains read sequences
    -p, --port
    port number
    -z, --zip
    zip output files
    -k, --kmer-length
    length of kmer
    -o, --output
    output folder
    --without-canonical-kmer
    do not use canonical kmer

```


### Kmer-Reads-Mapping

Find shared reads for kmers with kmer_read_mapping_$SURFIX where $SURFIX means mrmpi, mimir, mpi or upcxx. 

For example for mpi version:
```
    $./kmer_read_mapping_mpi -h
    
    -h, --help
    shows this help message
    -i, --input
    input folder which contains read sequences
    -p, --port
    port number
    -z, --zip
    zip output files
    -k, --kmer-length
    length of kmer
    -o, --output
    output folder
    --without-canonical-kmer
    do not use canonical kmer

```

### Edge Generating

Generate graph edges using edge_generating_$SURFIX where $SURFIX means mrmpi, mimir, mpi or upcxx. 

For example for mpi version:
```
    $./edge_generating_mpi -h
    
    -h, --help
    shows this help message
    -i, --input
    input folder which contains read sequences
    -p, --port
    port number
    -z, --zip
    zip output files
    -o, --output
    output folder
    --max-degree
    max_degree of a node; max_degree should be greater than 1
    --min-shared-kmers
    minimum number of kmers that two reads share. (note: this option does not work)
    
```

### Clustering
