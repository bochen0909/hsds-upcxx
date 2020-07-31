/*
 * kmer_read_mapping_upc.cpp
 *
 *  Created on: Jul 29, 2020
 *      Author:
 */

#include <string>
#include <sstream>
#include <exception>
#include <vector>
#include <unordered_set>
#include <algorithm>    // std::random_shuffle
#include <random>
#include <ctime>        // std::time
#include <cstdlib>
#include <string>
#include <iostream>
#include <unistd.h>
#include <upcxx/upcxx.hpp>

#include "argagg.hpp"
#include "gzstream.h"
#include "sparc/utils.h"
#include "kmer.h"
#include "sparc/log.h"
#include "sparc/config.h"
#include "upc/upchelper.h"
#include "upc/distrmap.h"

#define KMER_SEND_BATCH_SIZE (100*1000)

using namespace std;
using namespace sparc;

DistrMap<std::string, std::unordered_set<std::string>> *g_map = 0;
size_t g_n_sent = 0;

struct Config: public BaseConfig {
	bool without_canonical_kmer;
	int kmer_length;

	void print() {
		BaseConfig::print();
		myinfo("config: kmer_length=%d", kmer_length);
		myinfo("config: canonical_kmer=%d", !without_canonical_kmer ? 1 : 0);
	}
};

void check_arg(argagg::parser_results &args, char *name) {
	if (!args[name]) {
		cerr << name << " is missing" << endl;
		exit(-1);
	}

}

int run(const std::vector<std::string> &input, Config &config);

int main(int argc, char **argv) {
	upcxx::init();

	Config config;
	config.program = argv[0];
	config.rank = upcxx::rank_me();
	config.nprocs = upcxx::rank_n();
	config.mpi_hostname = sparc::get_hostname();
	set_spdlog_pattern(config.mpi_hostname.c_str(), config.rank);

	if (config.rank == 0) {
		myinfo("Welcome to Sparc!");
	}
	argagg::parser argparser { {

	{ "help", { "-h", "--help" }, "shows this help message", 0 },

	{ "input", { "-i", "--input" },
			"input folder which contains read sequences", 1 },

	{ "zip_output", { "-z", "--zip" }, "zip output files", 0 },

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

	config.without_canonical_kmer = args["without_canonical_kmer"];
	config.zip_output = args["zip_output"];

	check_arg(args, (char*) "kmer_length");
	config.kmer_length = args["kmer_length"].as<int>();
	if (config.kmer_length < 1) {
		cerr << "Error, length of kmer :  " << config.kmer_length << endl;
		return EXIT_FAILURE;
	}
	check_arg(args, (char*) "input");
	string inputpath = args["input"].as<string>();
	config.inputpath.push_back(inputpath);
	check_arg(args, (char*) "output");
	string outputpath = args["output"].as<string>();
	config.outputpath = outputpath;

	if (!dir_exists(inputpath.c_str())) {
		cerr << "Error, input dir does not exists:  " << inputpath << endl;
		return EXIT_FAILURE;
	}

	upcxx::barrier();

	if (upcxx::rank_me() == 0) {
		if (dir_exists(outputpath.c_str())) {
			cerr << "Error, output dir exists:  " << outputpath << endl;
			return EXIT_FAILURE;
		}
	}

	if (upcxx::rank_me() == 0) {
		if (make_dir(outputpath.c_str()) < 0) {
			cerr << "Error, mkdir dir failed for " << outputpath << endl;
			return EXIT_FAILURE;
		}
	}

	std::vector<std::string> myinput = get_my_files(inputpath);

	myinfo("#of my inputs = %ld", myinput.size());
	run(myinput, config);
	upcxx::finalize();

	return 0;
}

inline void map_line(const string &line, int kmer_length,
		bool without_canonical_kmer,
		std::vector<std::pair<std::string, std::string>> &kmers) {
	std::vector<std::string> arr;
	split(arr, line, "\t");
	if (arr.empty()) {
		return;
	}
	if (arr.size() != 3) {
		mywarn("Warning, ignore line (s=%ld): %s", arr.size(), line.c_str());
		return;
	}
	string seq = arr.at(2);
	trim(seq);
	string nodeid = trim_copy(arr.at(0));

	std::vector<std::string> v = generate_kmer(seq, kmer_length, "N",
			!without_canonical_kmer);

	for (size_t i = 0; i < v.size(); i++) {
		std::string s = kmer_to_base64(v[i]);
		kmers.push_back( { s, nodeid });
		g_n_sent++;
	}
}

int process_seq_file(const std::string &filepath, int kmer_length,
		bool without_canonical_kmer) {
	std::vector<std::pair<std::string, std::string>> kmers;
	if (endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			map_line(line, kmer_length, without_canonical_kmer, kmers);
			if (kmers.size() >= KMER_SEND_BATCH_SIZE) {
				g_map->value_set_insert(kmers).wait();
				kmers.clear();
			}
			upcxx::progress();
		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			map_line(line, kmer_length, without_canonical_kmer, kmers);
			if (kmers.size() >= KMER_SEND_BATCH_SIZE) {
				g_map->value_set_insert(kmers).wait();
				kmers.clear();
			}
			upcxx::progress();
		}
	}
	if (kmers.size() > 0) {
		g_map->value_set_insert(kmers).wait();
		kmers.clear();
	}
	upcxx::progress();

	return 0;
}

int run(const std::vector<std::string> &input, Config &config) {
	if (config.rank == 0) {
		config.print();
	}

	g_map = new DistrMap<std::string, std::unordered_set<std::string>>();

	for (size_t i = 0; i < input.size(); i++) {
		myinfo("processing %s", input.at(i).c_str());
		process_seq_file(input.at(i), config.kmer_length,
				config.without_canonical_kmer);
	}

	upcxx::barrier();

	myinfo("Total sent %ld kmers", g_n_sent);

	g_map->dump(config.get_my_output(), '\t',
			[](const std::unordered_set<std::string> &set) {
				std::stringstream ss;
				bool bfirst = true;
				for (const auto &s : set) {
					if (!bfirst) {
						ss << " ";
					}
					ss << s;
					bfirst = false;
				}
				return ss.str();
			});

	delete g_map;

	return 0;
}

