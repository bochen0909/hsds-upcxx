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

int kmer_length = -1;
bool without_canonical_kmer = false;

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
			"input folder which contains read sequences", 1 },

	{ "kmer_length", { "-k", "--kmer-length" }, "length of kmer", 1 },

	{ "output", { "-o", "--output" }, "output folder", 1 },

	{ "without_canonical_kmer", { "--without-canonical-kmer" },
			"do not use canonical kmer", 0 },

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
	run(input, outputpath, rank);
	MPI_Finalize();

	return 0;
}

void countword(Readable<char*, uint64_t> *input,
		Writable<char*, uint64_t> *output, void *ptr) {
	char *key = NULL;
	uint64_t val = 0;
	uint64_t count = 0;
	while (input->read(&key, &val) == true) {
		count += val;
	}
	output->write(&key, &count);
}

void combine(Combinable<char*, uint64_t> *combiner, char **key, uint64_t *val1,
		uint64_t *val2, uint64_t *rval, void *ptr) {
	*rval = *val1 + *val2;
}

void map_fun(Readable<char*, void> *input, Writable<char*, uint64_t> *output,
		void *ptr) {
	char *line = NULL;
	while (input->read(&line, NULL) == true) {
		std::vector<std::string> arr = split(line, "\t");
		if (arr.empty()) {
			continue;
		}
		if (arr.size() != 3) {
			cerr << "Warning, ignore line: " << line << endl;
			continue;
		}
		string seq = arr.at(2);
		trim(seq);

		std::vector<std::string> kmers = generate_kmer(seq, kmer_length, 'N',
				!without_canonical_kmer);
		for (int i = 0; i < kmers.size(); i++) {
			uint64_t one = 1;
			string &word = kmers.at(i);
			std::string encoded = kmer_to_base64(word);
			char *c = (char*) encoded.c_str();
			//char *c = (char*) word.c_str();
			output->write(&c, &one);
		}
	}
}

int run(const std::vector<std::string> &input, const string &outputpath,
		int rank) {

	MimirContext<char*, uint64_t, char*, void> *ctx = new MimirContext<char*,
			uint64_t, char*, void>(input, outputpath + "/kc", MPI_COMM_WORLD,
			combine);
	ctx->map(map_fun);
	uint64_t nunique = ctx->reduce(countword, NULL, true, "text");
	delete ctx;

	if (rank == 0)
		printf("nunique=%ld\n", nunique);
	return 0;
}

