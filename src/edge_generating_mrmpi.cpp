#include <string>
#include <exception>
#include <vector>
#include <algorithm>    // std::random_shuffle
#include <random>
#include <ctime>        // std::time
#include <cstdlib>
#include <string>
#include <iostream>
#include <unistd.h>

#include "argagg.hpp"
#include "utils.h"
#include "kmer.h"

#include "mapreduce.h"
#include "keyvalue.h"

using namespace std;
using namespace sparc;
using namespace MAPREDUCE_NS;

int nproc;

int max_degree = 100;
int min_shared_kmers = 2;

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
			"reads that a kmer shares. e.g. output from kmer_read_mapping", 1 },

	{ "output", { "-o", "--output" }, "output folder", 1 },

	{ "max_degree", { "--max-degree" },
			"max_degree of a node; max_degree should be greater than 1", 1 },

	{ "min_shared_kmers", { "--min-shared-kmers" },
			"minimum number of kmers that two reads share", 1 },

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

	if (args["max_degree"]) {
		max_degree = args["max_degree"].as<int>();
	}

	if (args["min_shared_kmers"]) {
		min_shared_kmers = args["min_shared_kmers"].as<int>();
	}

	int mrtimer = 1;
	if (args["mrtimer"]) {
		mrtimer = args["mrtimer"].as<int>();
	}

	int mrverbosity = 1;
	if (args["mrverbosity"]) {
		mrverbosity = args["mrverbosity"].as<int>();
		;
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
	MPI_Barrier(MPI_COMM_WORLD);
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
	std::vector<std::string> arr = split(line, "\t");
	if (arr.empty()) {
		return;
	}
	if (arr.size() != 2) {
		cerr << "Warning, ignore line: " << line << endl;
		return;
	}
	string seq = arr.at(1);
	trim(seq);
	arr = split(seq, " ");

	std::vector<uint32_t> reads;
	for (int i = 0; i < arr.size(); i++) {
		std::string a = arr.at(i);
		trim(a);
		if (!a.empty()) {
			reads.push_back(stoul(a));
		}
	}
	std::vector<std::pair<uint32_t, uint32_t> > edges = generate_edges(reads,
			max_degree);
	//cout << arr.size()<<  " gen " << edges.size() << endl;
	for (int i = 0; i < edges.size(); i++) {
		uint32_t a = edges.at(i).first;
		uint32_t b = edges.at(i).second;
		std::stringstream ss;
		if (a <= b) {
			ss << a << "\t" << b;
		} else {
			ss << b << "\t" << a;
		}

		string key = ss.str();
		kv->add((char*) key.c_str(), key.size() + 1, NULL, 0);
	}
}

void fileread(int itask, char *fname, KeyValue *kv, void *ptr) {
	// filesize = # of bytes in file

	struct stat stbuf;
	int flag = stat(fname, &stbuf);
	if (flag < 0) {
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
void sum(char *key, int keybytes, char *multivalue, int nvalues,
		int *valuebytes, KeyValue *kv, void *ptr) {
	if (nvalues >= min_shared_kmers) {
		kv->add(key, keybytes, (char*) &nvalues, sizeof(int));
	}
}

void filter_edges(uint64_t itask, char *key, int keybytes, char *value,
		int valuebytes, KeyValue *kv, void *ptr) {
	uint32_t n = (uint32_t) *value;
	if (n >= min_shared_kmers) {
		kv->add(key, keybytes, value, valuebytes);
	}
}

int run(const std::string &input, const string &outputpath, int rank,
		int mrtimer, int mrverbosity) {

	MapReduce *mr = new MapReduce(MPI_COMM_WORLD);
	mr->verbosity = mrverbosity;
	mr->timer = mrtimer;

	MPI_Barrier(MPI_COMM_WORLD);
	double tstart = MPI_Wtime();

	char *inputs[2] = { (char*) input.c_str() };
	uint64_t nparis = mr->map(1, &inputs[0], 0, 1, 0, fileread, NULL);
	int nfiles = mr->mapfilecount;
	mr->collate(NULL);
	uint64_t nedges = mr->reduce(sum, NULL);
	MapReduce *mr2 = NULL;
	if (true) {
		mr->write_str_int((char*) (outputpath + "/part").c_str());
	} else {
		mr2 = new MapReduce(MPI_COMM_WORLD);
		uint64_t nfiltered = 0;
		nfiltered = mr2->map(mr, filter_edges, NULL, 0);
		mr2->write_str_int((char*) (outputpath + "/part").c_str());
	}
	MPI_Barrier(MPI_COMM_WORLD);

	double tstop = MPI_Wtime();

	if (mr2) {
		delete mr2;
	}
	delete mr;

	if (rank == 0) {
		printf("%lu total node pairs, %lu filtered/total edges\n", nparis,
				nedges);
		printf("Time to process %d files on %d procs = %g (secs)\n", nfiles,
				nproc, tstop - tstart);
	}

	return 0;
}

