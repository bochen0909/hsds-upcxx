#include <string>
#include <exception>
#include <vector>
#include <algorithm>    // std::random_shuffle
#include <random>
#include <ctime>        // std::time
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "argagg.hpp"
#include "sparc/utils.h"
#include "kmer.h"

#include "mapreduce.h"
#include "keyvalue.h"

using namespace std;
using namespace sparc;
using namespace MAPREDUCE_NS;

int nproc;
int kmer_length = -1;
bool without_canonical_kmer = false;

void check_arg(argagg::parser_results &args, char *name) {
	if (!args[name]) {
		cerr << name << " is missing" << endl;
		exit(-1);
	}

}
int run(const std::string &input, const string &outputpath, int rank,
		int mrtimer, int mrverbosity);

int main(int argc, char **argv) {
	int rank, size;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	argagg::parser argparser { {

	{ "help", { "-h", "--help" }, "shows this help message", 0 },

	{ "input", { "-i", "--input" },
			"input folder which contains read sequences", 1 },

	{ "kmer_length", { "-k", "--kmer-length" }, "length of kmer", 1 },

	{ "output", { "-o", "--output" }, "output folder", 1 },

	{ "without_canonical_kmer", { "--without-canonical-kmer" },
			"do not use canonical kmer", 0 },

	{ "mrtimer", { "--mr-timer-flag" },
			"0 = none, 1 = summary, 2 = proc histograms", 1 },

	{ "mrverbosity", { "--verbose" },
			"0 = none, 1 = totals, 2 = proc histograms", 1 },

	} };

	argagg::parser_results args;
	try {
		args = argparser.parse(argc, argv);
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	if (args["help"]) {
		std::cerr << argparser;
		return EXIT_SUCCESS;
	}
	if (!args.pos.empty()) {
		std::cerr << "no positional argument is allowed" << endl;
		return EXIT_SUCCESS;
	}

	if (args["without_canonical_kmer"]) {
		without_canonical_kmer = true;
	}

	int mrtimer = 1;
	if (args["mrtimer"]) {
		mrtimer = args["mrtimer"].as<int>();
	}

	int mrverbosity = 1;
	if (args["mrverbosity"]) {
		mrverbosity = args["mrverbosity"].as<int>();
	}

	check_arg(args, (char*) "kmer_length");
	kmer_length = args["kmer_length"].as<int>();
	if (kmer_length < 1) {
		cerr << "Error, length of kmer :  " << kmer_length << endl;
		return EXIT_FAILURE;
	}
	check_arg(args, (char*) "input");
	string inputpath = args["input"].as<string>();
	check_arg(args, (char*) "output");
	string outputpath = args["output"].as<string>();

	if (!dir_exists(inputpath.c_str())) {
		cerr << "Error, input dir does not exists:  " << inputpath << endl;
		return EXIT_FAILURE;
	}
	if (rank == 0) {
		if (dir_exists(outputpath.c_str())) {
			cerr << "Error, output dir exists:  " << outputpath << endl;
			return EXIT_FAILURE;
		}
	}

	if (rank == 0) {
		if (make_dir(outputpath.c_str()) < 0) {
			cerr << "Error, mkdir dir failed for " << outputpath << endl;
			return EXIT_FAILURE;
		}
	}

	run(inputpath, outputpath, rank, mrtimer, mrverbosity);
	MPI_Finalize();

	return 0;
}
inline void process_line(const std::string &line, KeyValue *kv) {
	std::vector < std::string > arr = split(line, "\t");
	if (arr.empty()) {
		return;
	}
	if (arr.size() != 3) {
		cerr << "Warning, ignore line: " << line << endl;
		return;
	}
	string id = arr.at(0);
	trim(id);
	string seq = arr.at(2);
	trim(seq);

	std::vector < std::string > kmers = generate_kmer(seq, kmer_length, 'N',
			!without_canonical_kmer);
	for (int i = 0; i < kmers.size(); i++) {
		string &word = kmers.at(i);
		std::string encoded = kmer_to_base64(word);
		//cout << "MMM:" << id << "\t" << encoded << endl;
		kv->add((char*) encoded.c_str(), encoded.size() + 1, (char*) id.c_str(),
				id.size() + 1);
	}
}

void fileread(int itask, char *fname, KeyValue *kv, void *ptr) {
	if (!file_exists(fname)) {
		printf("ERROR: Could not query file size\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	fstream newfile;
	newfile.open(fname, ios::in); //open a file to perform read operation using file object
	if (newfile.is_open()) {   //checking whether the file is open
		string tp;
		while (getline(newfile, tp)) { //read data from file object and put it into string.
			process_line(tp, kv);
		}
		newfile.close(); //close the file object.
	}
}

/* ----------------------------------------------------------------------
 count word occurrence
 emit key = word, value = # of multi-values
 ------------------------------------------------------------------------- */
void sum_block(char *key, int keybytes, char *multivalue, int nvalues,
		int *valuebytes, KeyValue *kv, void *ptr) {
	std::stringstream ss;
	for (int i = 0; i < nvalues; i++) {
		if (i > 0) {
			ss << " ";
		}

		int l = *valuebytes;
		ss << multivalue;
		valuebytes++;
		multivalue += l;
	}
	std::string v = ss.str();
	kv->add(key, keybytes, (char*) v.c_str(), v.size() + 1);

}
void sum(char *key, int keybytes, char *multivalue, int nvalues,
		int *valuebytes, KeyValue *kv, void *ptr) {
	if (nvalues > 0) {
		sum_block(key, keybytes, multivalue, nvalues, valuebytes, kv, ptr);
	} else {
		MapReduce *mr = (MapReduce*) valuebytes;
		int nblocks;
		uint64_t nvalues_total = mr->multivalue_blocks(nblocks);
		for (int iblock = 0; iblock < nblocks; iblock++) {
			int nv = mr->multivalue_block(iblock, &multivalue, &valuebytes);
			sum_block(key, keybytes, multivalue, nv, valuebytes, kv, ptr);
		}
	}
}

int run(const std::string &input, const string &outputpath, int rank,
		int mrtimer, int mrverbosity) {

	MapReduce *mr = new MapReduce(MPI_COMM_WORLD);
	mr->verbosity = mrverbosity;
	mr->timer = mrtimer;

	MPI_Barrier (MPI_COMM_WORLD);
	double tstart = MPI_Wtime();

	char *inputs[2] = { (char*) input.c_str() };
	int nwords = mr->map(1, &inputs[0], 0, 1, 0, fileread, NULL);
	int nfiles = mr->mapfilecount;
	mr->collate(NULL);
	int nunique = mr->reduce(sum, NULL);

	mr->write_str_str((char*) (outputpath + "/part").c_str());
	MPI_Barrier(MPI_COMM_WORLD);
	double tstop = MPI_Wtime();

	delete mr;

	if (rank == 0) {
		printf("%d total kmers, %d unique kmers\n", nwords, nunique);
		printf("Time to process %d files on %d procs = %g (secs)\n", nfiles,
				nproc, tstop - tstart);
	}

	return 0;
}

