/*
 * edge_generating_upc.cpp
 *
 *  Created on: Jul 29, 2020
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
#include <upcxx/upcxx.hpp>

#include "argagg.hpp"
#include "gzstream.h"
#include "sparc/utils.h"
#include "kmer.h"
#include "sparc/log.h"
#include "sparc/config.h"
#include "upc/upchelper.h"
#include "upc/distrmap.h"

using namespace std;
using namespace sparc;

#define KMER_SEND_BATCH_SIZE (100*1000)

DistrMap<std::string, uint32_t> *g_map = 0;
size_t g_n_sent = 0;

struct Config: public BaseConfig {
	size_t min_shared_kmers;
	size_t max_degree;
	int n_iteration;

	void print() {
		BaseConfig::print();
		myinfo("config: n_iteration=%ld", n_iteration);
		myinfo("config: max_degree=%ld", max_degree);
		myinfo("config: min_shared_kmers=%ld", min_shared_kmers);
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

	argagg::parser argparser {
			{

			{ "help", { "-h", "--help" }, "shows this help message", 0 },

			{ "input", { "-i", "--input" },
					"input folder which contains read sequences", 1 },

			{ "n_iteration", { "-n" }, "number of iteration", 1 },

			{ "zip_output", { "-z", "--zip" }, "zip output files", 0 },

			{ "output", { "-o", "--output" }, "output folder", 1 },

			{ "max_degree", { "--max-degree" },
					"max_degree of a node; max_degree should be greater than 1",
					1 },

					{ "min_shared_kmers", { "--min-shared-kmers" },
							"minimum number of kmers that two reads share. (note: this option does notework)",
							1 },

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

	config.max_degree = args["max_degree"].as<int>(100);
	config.n_iteration = args["n_iteration"].as<int>(1);
	config.min_shared_kmers = args["min_shared_kmers"].as<int>(2);
	config.zip_output = args["zip_output"];

	if (!args.pos.empty()) {
		config.inputpath = args.all_as<string>();
	}

	if (args["input"]) {
		string a = args["input"].as<string>();
		config.inputpath.push_back(a);
	}

	if (!config.inputpath.empty()) {
		bool b_error = false;
		for (size_t i = 0; i < config.inputpath.size(); i++) {
			if (!dir_exists(config.inputpath.at(i).c_str())) {
				cerr << "Error, input dir does not exists:  "
						<< config.inputpath.at(i) << endl;
				b_error = true;
			}
		}
		if (b_error) {
			return EXIT_FAILURE;
		}

	} else {
		cerr << "Error, no inputs specified. " << endl;
		return EXIT_FAILURE;
	}

	check_arg(args, (char*) "output");
	string outputpath = args["output"].as<string>();
	config.outputpath = outputpath;

	upcxx::barrier();

	if (config.rank == 0) {
		if (dir_exists(outputpath.c_str())) {
			cerr << "Error, output dir exists:  " << outputpath << endl;
			return EXIT_FAILURE;
		}
	}

	if (config.rank == 0) {
		if (make_dir(outputpath.c_str()) < 0) {
			cerr << "Error, mkdir dir failed for " << outputpath << endl;
			return EXIT_FAILURE;
		}
	}

	std::vector<std::string> myinput = get_my_files(config.inputpath);
	myinfo("#of my inputs = %ld", myinput.size());
	run(myinput, config);
	upcxx::finalize();

	return 0;
}

inline void map_line(int iteration, int n_interation, const std::string &line,
		size_t min_shared_kmers, size_t max_degree,
		std::vector<std::string> &edges_output) {
	std::vector<std::string> arr = split(line, "\t");
	if (arr.empty()) {
		return;
	}
	if (arr.size() != 2) {
		mywarn("Warning, ignore line: %s", line.c_str());
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

	for (int i = 0; i < edges.size(); i++) {
		uint32_t a = edges.at(i).first;
		uint32_t b = edges.at(i).second;
		if ((a + b) % n_interation == iteration) {
			std::stringstream ss;
			if (a <= b) {
				ss << a << " " << b;
			} else {
				ss << b << " " << a;
			}
			string s = ss.str();
			edges_output.push_back(s);

			if (edges_output.size() >= KMER_SEND_BATCH_SIZE) {
				g_map->incr(edges_output).wait();
				edges_output.clear();
			}


			g_n_sent++;
		}
	}
}

int process_krm_file(int iteration, int n_interation,
		const std::string &filepath, size_t min_shared_kmers,
		size_t max_degree) {
	std::vector<std::string> edges;
	if (endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			map_line(iteration, n_interation, line, min_shared_kmers,
					max_degree, edges);
			upcxx::progress();
		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			map_line(iteration, n_interation, line, min_shared_kmers,
					max_degree, edges);
			upcxx::progress();
		}
	}
	if (edges.size() > 0) {
		g_map->incr(edges).wait();
		edges.clear();
	}

	return 0;
}

int run(int iteration, const std::vector<std::string> &input, Config &config) {

	for (size_t i = 0; i < input.size(); i++) {
		myinfo("processing %s", input.at(i).c_str());
		process_krm_file(iteration, config.n_iteration, input.at(i),
				config.min_shared_kmers, config.max_degree);
	}

	upcxx::barrier();

	myinfo("Total sent %ld kmers", g_n_sent);

	g_map->dump(config.get_my_output(iteration), ' ', [](uint32_t v) {
		return v;
	});

	return 0;
}

int run(const std::vector<std::string> &input, Config &config) {
	if (config.rank == 0) {
		config.print();
	}

	g_map = new DistrMap<std::string, uint32_t>();

	for (int i = 0; i < config.n_iteration; i++) {
		if (config.rank == 0) {
			myinfo("Starting iteration %d", i);
		}

		run(i, input, config);
		upcxx::barrier();
	}

	delete g_map;
	return 0;

}
