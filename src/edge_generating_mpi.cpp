/*
 * edge_generating_mpi.cpp
 *
 *  Created on: Jul 13, 2020
 *      Author:
 */

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
#include <mpi.h>

#include "argagg.hpp"
#include "sparc/utils.h"
#include "kmer.h"

using namespace std;
using namespace sparc;

struct Config {
	bool without_canonical_kmer;
	int max_degree;
	int min_shared_kmers;
	int kmer_length;
	string inputpath;
	string outputpath;
	int rank;
	int nprocs;

};
void check_arg(argagg::parser_results &args, char *name) {
	if (!args[name]) {
		cerr << name << " is missing" << endl;
		exit(-1);
	}

}

int run(const std::vector<std::string> &input, const string &outputpath,
		Config &config);

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

	{ "max_degree", { "--max-degree" },
			"max_degree of a node; max_degree should be greater than 1", 1 },

	{ "min_shared_kmers", { "--min-shared-kmers" },
			"minimum number of kmers that two reads share", 1 },

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

	Config config;
	config.rank = rank;
	config.nprocs = size;

	if (args["without_canonical_kmer"]) {
		config.without_canonical_kmer = true;
	} else {
		config.without_canonical_kmer = false;
	}

	if (args["max_degree"]) {
		config.max_degree = args["max_degree"].as<int>();
	} else {
		config.max_degree = 100;
	}

	if (args["min_shared_kmers"]) {
		config.min_shared_kmers = args["min_shared_kmers"].as<int>();
	} else {
		config.min_shared_kmers = 2;
	}

	check_arg(args, (char*) "kmer_length");
	config.kmer_length = args["kmer_length"].as<int>();
	if (config.kmer_length < 1) {
		cerr << "Error, length of kmer :  " << config.kmer_length << endl;
		return EXIT_FAILURE;
	}
	check_arg(args, (char*) "input");
	string inputpath = args["input"].as<string>();
	config.inputpath = inputpath;
	check_arg(args, (char*) "output");
	string outputpath = args["output"].as<string>();
	config.outputpath = outputpath;

	if (!dir_exists(inputpath.c_str())) {
		cerr << "Error, input dir does not exists:  " << inputpath << endl;
		return EXIT_FAILURE;
	}

	MPI_Barrier(MPI_COMM_WORLD);
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

	//std::vector<std::string> input = list_dir(inputpath.c_str());
	std::vector<std::string> input;
	input.push_back(inputpath);
	if (input.size() == 0) {
		cerr << "Error, input dir is empty " << outputpath << endl;
		return EXIT_FAILURE;
	} else {
		if (rank == 0) {
			cout << "#of inputs: " << input.size() << endl;
		}
	}
	run(input, outputpath, config);
	MPI_Finalize();

	return 0;
}

int run(const std::vector<std::string> &input, const string &outputpath,
		Config &config) {

	return 0;
}

