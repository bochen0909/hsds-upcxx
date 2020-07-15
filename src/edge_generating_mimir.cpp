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
#include "sparc/utils.h"
#include "kmer.h"

#include "mimir.h"

using namespace MIMIR_NS;
using namespace std;
using namespace sparc;

int max_degree = 100;
int min_shared_kmers = 2;

void check_arg(argagg::parser_results &args, char *name) {
	if (!args[name]) {
		cerr << name << " is missing" << endl;
		exit(-1);
	}

}
int run(const std::vector<std::string> &input, const string &outputpath,
		int rank);
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

	if (!args.pos.empty()) {
		std::cerr << "no positional argument is allowed" << endl;
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

	if (rank == 0) {
		if (!dir_exists(inputpath.c_str())) {
			cerr << "Error, input dir does not exists:  " << inputpath << endl;
			return EXIT_FAILURE;
		}
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
	run(input, outputpath, rank);
	MPI_Finalize();

	return 0;
}

void sum_edge_weight(Readable<char*, uint32_t> *input,
		Writable<char*, uint32_t> *output, void *ptr) {
	char *key = NULL;
	uint32_t val = 0;
	uint32_t count = 0;
	while (input->read(&key, &val) == true) {
		count += val;
	}
	if (count >= min_shared_kmers) {
		output->write(&key, &count);
	}
}

void combine(Combinable<char*, uint32_t> *combiner, char **key, uint32_t *val1,
		uint32_t *val2, uint32_t *rval, void *ptr) {
	*rval = *val1 + *val2;
}

inline void process_line(const std::string &line,
		Writable<char*, uint32_t> *output) {
	std::vector<std::string> arr = split(line, " ");

	if (arr.size() < 2) {
		cerr << "Warning, ignore line: " << line << endl;
		return;
	}

	std::vector<uint32_t> reads;
	for (int i = 1; i < arr.size(); i++) {
		std::string a = arr.at(i);
		trim(a);
		if (!a.empty()) {
			reads.push_back(stoul(a));
		}
	}
	std::vector<std::pair<uint32_t, uint32_t> > edges = generate_edges(reads,
			max_degree);
	for (int i = 0; i < edges.size(); i++) {
		uint32_t a = edges.at(i).first;
		uint32_t b = edges.at(i).second;
		std::stringstream ss;
		if (a <= b) {
			ss << a << " " << b;
		} else {
			ss << b << " " << a;
		}

		string key = ss.str();

		uint32_t one = 1;
		char *c = (char*) key.c_str();
		output->write(&c, &one);
	}
}

//map line to (kmer,readid)
void line_parse_map_fun(Readable<char*, void> *input,
		Writable<char*, uint32_t> *output, void *ptr) {
	char *line = NULL;
	while (input->read(&line, NULL) == true) {
		process_line(line, output);
	}
}

int run(const std::vector<std::string> &input, const string &outputpath,
		int rank) {

	MimirContext<char*, uint32_t, char*, void, char*, uint32_t> *ctx =
			new MimirContext<char*, uint32_t, char*, void, char*, uint32_t>(
					input, outputpath + "/edge", MPI_COMM_WORLD, combine);
	uint64_t n_input = ctx->map(line_parse_map_fun);
	uint64_t n_edges = ctx->reduce(sum_edge_weight, NULL, true, "text");
	delete ctx;

	if (rank == 0) {
		printf(" n_edges=%ld\n", n_edges);
	}
	return 0;
}

