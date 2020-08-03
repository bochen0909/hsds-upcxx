# Sparc in MPI

This site provides MPI based implementation of **Sparc** (refer to [paper](https://academic.oup.com/bioinformatics/article/35/5/760/5078476) and [sources](https://bitbucket.org/LizhenShi/sparc/src/master/)).

Four versions are provided: *mrmpi*, *mimir*, *mpi* and *upcxx*. Each version provides a few programs that coresponding the to steps in [Sparc](https://bitbucket.org/LizhenShi/sparc/src/master/README.md) .

**Sparc_mrmpi** utilized map-reduce lib of [MapReduce-MPI Library](https://mapreduce.sandia.gov/). Since Sparc is based on [Apache Spark](https://spark.apache.org/) whose foundation is map-reduce, this is a straightforward "translation".

**Sparc_mimir** was written based on another MPI map-reduce lib, [Mimir](https://github.com/TauferLab/Mimir), which performs better than sparc_mrmpi. It is also a straightforward "translation".

**Sparc_mpi** is a re-implementation that based on MPI and [ZMQ](https://zeromq.org/) using client-server p2p communication. It preforms much better than sparc_mrmpi and sparc_mimir.

**Sparc_upcxx** is a re-implementation that uses [UPC++](https://bitbucket.org/berkeleylab/upcxx/wiki/Home). It performs similar or even better than sparc_mpi. However I find it might be tricky to run a upcxx program since the data transfer is hidden from users. I found failure sometimes when lots of rpc calls were performed asynchronously. 

When running the programs, make sure there are enough nodes to hold all the data in memory. Although some programs support storing temporary data in disk, but it will make the progress really slow.

### Example Runs

Please find sbatch scripts of sample runs on [LAWRENCIUM](https://sites.google.com/a/lbl.gov/high-performance-computing-services-group/lbnl-supercluster/lawrencium) in [misc/example](misc/example) folder.



##Installation



First clone the code

```
    git clone https://github.com/Lizhen0909/sparc-mpi.git
    cd sparc-mpi && git submodule update --init --recursive
```

All the versions are independent from each other, you may choose to only build the version that you are interested in.


### Mimir Version

#### Requires
* Linux 
* mpi >= 3.0
* c++ standard >= 11 
* autoconf>= 2.67
* automake >= 1.15
* libtool >= 2.4.4


#### Build
```
    mkdir build && cd build
    cmake -DBUILD_SPARC_MIMIR=ON ..
    make sparc_mimir
```

### MRMPI Version

#### Requires
* Linux 
* mpi >= 3.0
* c++ standard >= 11 


#### Build
```
    mkdir build && cd build
    cmake -DBUILD_SPARC_MRMPI=ON ..
    make sparc_mrmpi

```


### MPI Version

#### Requires
* Linux 
* mpi >= 3.0
* c++ standard >= 11 


#### Build
```
    mkdir build && cd build
    cmake -DBUILD_SPARC_MPI=ON ..
    make sparc_mpi
```
### UPC++ Version

#### Requires
* Linux 
* mpi >= 3.0
* c++ standard >= 11 
* upcxx 2020.3.2


#### Build

Install UPC++ is beyond this scope. 
Set environment *UPCXX_INSTALL* helps cmake to find upcxx.
Also remember set UPCXX_NETWORK to your network, otherwise smp is most likely be used. 
```
    mkdir build && cd build
    cmake -DBUILD_SPARC_UPCXX=ON ..
    make sparc_upcxx
```


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

### LPA Clustering

Find clusters by Label Propagation Algorithm. 
In this step there are two options: PowerGraph LPA or *lpav1_mpi/upcxx*. 

*lpav1_mpi/upcxx* is my implementation of LPA in MPI or UPC++, which is build in the corresponding targets.

PowerGraph(https://github.com/jegonzal/PowerGraph) also provide LPA program which can be got from github.

*Remark:* Since this step is purely graph clustering, actually any graph clustering algorithms can be used providing it can work on graphs of billions edges.

