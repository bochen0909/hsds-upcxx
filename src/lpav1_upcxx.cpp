/*
 * kmer_counting_upc.cpp
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
#include "upcxx/distr_lpa_graph.h"
#include "upcxx/upchelper.h"

using namespace std;
using namespace sparc;

#define KMER_SEND_BATCH_SIZE (1000*1000)

DistrGraph *g_graph = 0;

size_t g_n_sent = 0;

struct Config: public BaseConfig {
	int n_iteration;
	uint32_t smin;
	bool weighted;

	void print() {
		BaseConfig::print();
		myinfo("config: smin=%ld", smin);
		myinfo("config: n_iteration=%d", n_iteration);
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

	{ "zip_output", { "-z", "--zip" }, "zip output files", 0 },

	{ "output", { "-o", "--output" }, "output folder", 1 },

	{ "n_iteration", { "-n", }, "how many iterations to run", 1 },

	{ "weighted", { "-w", }, "weighted graph", 0 },

	{ "smin", { "--smin" },
			"minimum node id for short reads, default 4294967295", 1 }, } };

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

	config.smin = args["smin"].as<uint32_t>(-1);
	config.n_iteration = args["n_iteration"].as<uint32_t>(300);
	config.zip_output = args["zip_output"];
	config.weighted = args["weighted"];

	if (args.pos.empty()) {
		std::cerr << "no input files are provided" << endl;
		return EXIT_SUCCESS;
	} else {
		config.inputpath = args.all_as<string>();
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
	}

	string outputpath = args["output"].as<string>();
	config.outputpath = outputpath;

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

	std::vector<std::string> myinput = get_my_files(config.inputpath);
	myinfo("#of my input: %ld", myinput.size());
	run(myinput, config);

	upcxx::finalize();

	return 0;
}

inline int pasrse_graph_file_line(const std::string &line, bool weighted,
		std::vector<std::tuple<uint32_t, uint32_t, float>> &edges) {

	if (line.empty()) {
		return 0;
	}
	if (isdigit(line.at(0))) {
		uint32_t a;
		uint32_t b;
		float w = 1;
		stringstream ss(line);
		ss >> a >> b;
		if (weighted) {
			ss >> w;
		}
		edges.push_back( { a, b, w });
		g_n_sent++;
		return 2;
	} else if ('#' == line.at(0)) {
		return 0;
	} else {
		mywarn("malicious line: %s", line.c_str());
		return 0;
	}
}

int process_graph_file(const std::string &filepath, bool weighted) {
	std::vector<std::tuple<uint32_t, uint32_t, float>> edges;
	if (sparc::endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			pasrse_graph_file_line(line, weighted, edges);
			if (edges.size() >= KMER_SEND_BATCH_SIZE) {
				g_graph->add_edges(edges).wait();
				edges.clear();
			}

		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			pasrse_graph_file_line(line, weighted, edges);
			if (edges.size() >= KMER_SEND_BATCH_SIZE) {
				g_graph->add_edges(edges).wait();
				edges.clear();
			}
		}
	}
	if (edges.size() > 0) {
		g_graph->add_edges(edges).wait();
		edges.clear();
	}

	return 0;
}

int make_graph(const std::vector<std::string> &input, bool weighted) {

	for (size_t i = 0; i < input.size(); i++) {
		myinfo("processing %s", input.at(i).c_str());
		process_graph_file(input.at(i), weighted);
	}
	return 0;

}

void run_iteration(int i) {
	if (upcxx::rank_me() == 0) {
		myinfo("starting updating for iteration %d", i);
	}

	g_graph->query_and_update_nodes();

	upcxx::barrier();

	g_graph->set_local_neighbor_changed(false);

	upcxx::barrier();

	if (upcxx::rank_me() == 0) {
		myinfo("starting update changed info for iteration %d", i);
	}

	g_graph->notify_changed_nodes();

}

int run(const std::vector<std::string> &input, Config &config) {

	if (config.rank == 0) {
		config.print();
	}

	g_graph = new DistrGraph(config.smin);

	make_graph(input, config.weighted);
	upcxx::barrier();
	g_graph->finalize();
	upcxx::barrier();

	if (config.rank == 0) {
		myinfo("Finished graph initialization and totally sent %ld records",
				g_n_sent);
	}

	//ready to run
	for (int i = 0; i < config.n_iteration; i++) {
		run_iteration(i);
		upcxx::barrier();

		if (g_graph->check_finished()) {
			if (config.rank == 0) {
				myinfo("Finished at iteration %d ", i);
			}
			break;
		}
	}

	g_graph->dump_clusters(config.get_my_output(), '\t');

	delete g_graph;

	return 0;

}
