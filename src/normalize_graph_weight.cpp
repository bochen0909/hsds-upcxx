/*
 * make_kgraph.cpp
 *
 *  Created on: Jul 23, 2021
 *      Author: Bo Chen
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

#include "argagg.hpp"
#include "gzstream.h"
#include "utils.h"
#include "kmer.h"
#include "log.h"
#include "config.h"

using namespace std;
using namespace sparc;

size_t g_n_sent = 0;

template<>
struct std::hash<std::pair<uint32_t, uint32_t>> {
	inline size_t operator()(const std::pair<uint32_t, uint32_t> &val) const {
		size_t h = val.first; //not safe assume uint32_t is 8 bits;
		return (h << 32) + val.second;
	}
};

struct Config: public BaseConfig {

	void print() {
		BaseConfig::print();
	}
};

void check_arg(argagg::parser_results &args, char *name) {
	if (!args[name]) {
		cerr << name << " is missing" << endl;
		exit(-1);
	}

}
void get_all_files(const std::vector<std::string> &folders,
		std::vector<std::string> &ret) {

	for (size_t i = 0; i < folders.size(); i++) {
		auto v = list_dir(folders.at(i).c_str());
		ret.insert(ret.end(), v.begin(), v.end());
	}
	sort(ret.begin(), ret.end());
	ret.erase(unique(ret.begin(), ret.end()), ret.end());
}

std::vector<std::string> get_all_files(
		const std::vector<std::string> &folders) {
	std::vector<std::string> ret;
	get_all_files(folders, ret);
	return ret;
}

int run(const std::vector<std::string> &input, Config &config);

int main(int argc, char **argv) {

	Config config;
	config.program = argv[0];
	config.mpi_hostname = sparc::get_hostname();
	set_spdlog_pattern(config.mpi_hostname.c_str(), 0);

	argagg::parser argparser { {

	{ "help", { "-h", "--help" }, "shows this help message", 0 },

	{ "zip_output", { "-z", "--zip" }, "zip output files", 0 },

	{ "output", { "-o", "--output" }, "output file", 1 },

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

	config.zip_output = args["zip_output"];

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

	if (file_exists(outputpath.c_str())) {
		cerr << "Warning, output file is overwritten:  " << outputpath << endl;
	}

	std::vector<std::string> myinput = get_all_files(config.inputpath);
	myinfo("#of my input: %ld", myinput.size());
	run(myinput, config);

	return 0;
}

inline int pasrse_graph_file_line(const std::string &line, bool weighted,
		std::unordered_map<std::pair<uint32_t, uint32_t>, float> &edges) {

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
		if (a < b) {
			edges[std::make_pair(a, b)] = w;
		} else {
			edges[std::make_pair(b, a)] = w;
		}
		g_n_sent++;
		return 2;
	} else if ('#' == line.at(0)) {
		return 0;
	} else {
		mywarn("malicious line: %s", line.c_str());
		return 0;
	}
}

int process_graph_file(const std::string &filepath,
		std::unordered_map<std::pair<uint32_t, uint32_t>, float> &edges) {
	bool weighted = true;
	if (sparc::endswith(filepath, ".gz")) {
		igzstream file(filepath.c_str());
		std::string line;
		while (std::getline(file, line)) {
			pasrse_graph_file_line(line, weighted, edges);

		}
	} else {
		std::ifstream file(filepath);
		std::string line;
		while (std::getline(file, line)) {
			pasrse_graph_file_line(line, weighted, edges);

		}
	}

	return 0;
}

int make_graph(const std::vector<std::string> &input,
		std::unordered_map<std::pair<uint32_t, uint32_t>, float> &edges) {

	for (size_t i = 0; i < input.size(); i++) {
		myinfo("processing %s", input.at(i).c_str());
		process_graph_file(input.at(i), edges);
	}
	return 0;

}

void _normalize(
		std::unordered_map<std::pair<uint32_t, uint32_t>, float> &edgelist) {
	if (edgelist.empty()) {
		return;
	}
	float sum = 0;
	for (auto &kv : edgelist) {
		sum += kv.second;
	}
	float avg = float(sum / edgelist.size());

	for (auto &kv : edgelist) {
		kv.second /= avg;
	}
}

template<typename Iter>
void write_edges_text(const std::string &path, Iter it, Iter end) {

	auto text_file = fopen(path.c_str(), "wt");
	for (; it != end; ++it) {
		auto &kv = *it;
		fprintf(text_file, "%lu ", (size_t) kv.first.first);
		fprintf(text_file, "%lu ", (size_t) kv.first.second);
		fprintf(text_file, "%f", (double) kv.second);
		fprintf(text_file, "\n");
	}
	fclose(text_file);
}

int run(const std::vector<std::string> &input, Config &config) {

	std::unordered_map<std::pair<uint32_t, uint32_t>, float> edges;
	make_graph(input, edges);
	myinfo("normalized by average weights");

	_normalize(edges);

	if (true) {
		auto path = config.outputpath;
		write_edges_text(path, edges.begin(), edges.end());
	}

	return 0;

}
