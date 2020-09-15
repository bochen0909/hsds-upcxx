import os,sys
import gzip
from tqdm import tqdm
from pylshvec import *

LOCAL="/tmp/"
print("using", LOCAL)

#os.environ['JAVA_HOME']='/global/software/sl-7.x86_64/modules/langs/java/1.8.0_121'


lshvecdir=os.path.join(os.environ["HOME"], 'scratch', "lshvec" )
n_thread=20

def get_model():
    global n_thread
    set_lshvec_jar_path(os.path.join(lshvecdir , "jlshvec-assembly-0.1.jar"))
    add_java_options("-Xmx48G")

    model= LSHVec(model_file=os.path.join(lshvecdir , "model/genbank_model_gs_k23_l150/model_299"), 
              hash_file=os.path.join(lshvecdir , "lsh/lsh_nt_NonEukaryota_k23_h25.crp"),
                max_results=500, 
                 num_thread=n_thread, only_show_main_tax=True
             )
    return model

clu_fastq_files={}
basename=None
g_wrong=0

def read_fastq(fpath):
    with gzip.open(fpath) as f:
        fastq=[]
        for line in f:
            fastq.append(line)
            if len(fastq)==8:
                firstid = fastq[0].split()[0]
                secondid = fastq[4].split()[0]
                if firstid==secondid:
                    yield fastq
                    fastq=[]
                else: #something wrong
                    #print("wrong", firstid, secondid)
                    fastq=fastq[4:]
                    global g_wrong
                    g_wrong+=1
                    
def get_clu_fastq_file(cid):
    global basename
    if cid not in clu_fastq_files:
        fpath=LOCAL+"/{}/ill_cid_{}.fastq".format(basename, cid)
        f = open(fpath,'wb')
        clu_fastq_files[cid]=f
    return clu_fastq_files[cid]

def write_fastq(f, l):
    for u in l:
        f.write(u)

global_rank={}
def get_rank(i):
    if i not in global_rank:
        global model
        global_rank[i]=model.getRank(i)
    return global_rank[i]

def handle_result(r):
    l=list(r.items())
    l=sorted(l, key=lambda u:u[1], reverse=True)
        
    l=[x[0] for x in l if get_rank(x[0])=='species'][:5]
    return l

def handle_batch(batch):
    seqs=[]
    for a in batch:
        seqs.append(a[1][:-1].decode('ascii')+"NNN"+a[5][:-1].decode('ascii'))
    
    rs=model.predict(seqs)
    n=len(batch)
    for i in range(n):
        seq=batch[i]
        r=rs[i]
        cids = handle_result(r)
        for cid in cids:
            write_fastq(get_clu_fastq_file(cid),seq)

def run(fname,basename):
    global n_thread
    batch=[]
    batch_size=n_thread*1000
    generator=read_fastq(fname)
    for a in tqdm(generator):
        assert a[0][0]==64 
        assert a[4][0]==64
        batch.append(a)
        if len(batch)>=batch_size:
            handle_batch(batch)
            batch=[]
    if len(batch)>0:
        handle_batch(batch)
        batch=[]


if __name__ == "__main__":
    fname=sys.argv[1]
    print("processing", fname)
    os.path.exists(fname) and fname.endswith('.fastq.gz')
    model = get_model()
    basename=fname.split('/')[-1].replace('.fastq.gz',"")
    run(fname,basename)
