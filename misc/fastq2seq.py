#!/usr/bin/env python

# prepare data for Spark-shared-kmer
# input is fastq.gz files from Illumina
# Read pairs are already joined, if not, the pair will be joined
# output .seq file

import os
import gzip
import sys, getopt
import random
import re
from tqdm import tqdm



sub_re = re.compile(r"[^ACGT]")

def extract_data(in_file, out_file, paired, shuffle, batch_size, iscami2=False):
    """
    Extract certain number of bases from a list of gzip files
    output format: no    ID    Seq
    """
    if out_file.endswith('.gz'):
        out = gzip.open(out_file, 'wt', compresslevel=1)
    elif out_file.endswith('.zst'):
        out = ZstdFile(out_file, 'w')    
        out.__enter__();    
    else:
        out = open(out_file, 'wt')
    seq_list = []
    no_seq = 1
    if paired:
        """
            join pairs together with 'NNNN'. use first read ID as the ID
        """
        lineno = 0    
        if in_file[-3:] == ".gz":
            seq = gzip.open(in_file, 'rt' )
        else:
            seq = open(in_file, 'rt')
        for line in tqdm(seq):
            lineno += 1
            if lineno % 8 == 1: 
                seqID = line[1:-1]
                newseqid = seqID.split()[0]
            elif lineno % 8 == 2: 
                seqSeq1 = line[:-1]
            elif iscami2 and  lineno % 8 == 5:
                seqID2 = line[1:-1]
                newseqid, b = seqID.split("/")
                c, d = seqID2.split("/")
                assert newseqid == c and int(b) == 1 and int(d) == 2, "not paired `{}` vs `{}`".format(seqID, seqID2)                
            elif lineno % 8 == 6:
                seqSeq2 = line[:-1]
                seq_list.append(str(no_seq) + "\t" + newseqid + "\t" + sub_re.sub("N", seqSeq1.upper()) + 'NNNN' + sub_re.sub("N", seqSeq2.upper()) + "\n")
                no_seq += 1
            if len(seq_list) > batch_size:
                if shuffle:
                    random.shuffle(seq_list)
                out.writelines(seq_list)
                seq_list = []
        seq.close()

    else:
        lineno = 0  
        if in_file[-3:] == ".gz":
            seq = gzip.open(in_file, 'rt')
        else:
            seq = open(in_file, 'rt')
        for line in tqdm(seq):
            lineno += 1
            if lineno % 4 == 1: 
                seqID = line[1:-1]
                seqID = seqID.split()[0]
            if lineno % 4 == 2: 
                seq_list.append(str(no_seq) + "\t" + seqID + "\t" + sub_re.sub("N", line[:-1].upper()) + "\n")
                no_seq += 1     
            if len(seq_list) > batch_size:
                if shuffle:
                    random.shuffle(seq_list)
                out.writelines(seq_list)
                seq_list = []
        seq.close()
    
    if len(seq_list) > 0:
        if shuffle:
            random.shuffle(seq_list)
        out.writelines(seq_list)

    out.close()


def main(argv):
    in_file = ''
    paired = False
    shuffle = False
    iscami2 = False
    out_file = ''
    batch_size = 1000 * 1000
    help_msg = sys.argv[0] + ' -i <fastq_file> -p <1 is paired, 0 otherwise> -o <outfile> -s <1 to shuffle, 0 otherwise> --cami2 [--batch n]'
    if len(argv) < 2:
        print (help_msg)
        sys.exit(2) 
    else:
        try:
            opts, args = getopt.getopt(argv, "hi:p:o:s:", ["in_file=", "paired=", "out_file=", "shuffle=", 'cami2', 'batch='])
        except getopt.GetoptError:
            print (help_msg)
            sys.exit(2)
        for opt, arg in opts:
            if opt == '-h':
                print (help_msg)
                sys.exit()
            elif opt in ("-i", "--in_file"):
                in_file = arg
            elif opt in ("-p", "--paired"):
                if int(arg) == 1 :
                    paired = True
            elif opt in ("--batch"):
                batch_size = int(arg) 
                assert batch_size > 0
            elif opt in ("-s", "--shuffle"):
                if int(arg) == 1 :
                    shuffle = True
            elif opt in ("--cami2"):
                iscami2 = True                    
            elif opt in ("-o", "--out_file"):
                out_file = arg  
    extract_data(in_file, out_file, paired, shuffle, batch_size, iscami2=iscami2)


if __name__ == "__main__":
    main(sys.argv[1:])

