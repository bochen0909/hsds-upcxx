/*
 * edge_generating_mpi.cpp
 *
 *  Created on: Jul 15, 2020
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
#include "gzstream.h"
#include "sparc/utils.h"
#include "kmer.h"
#include "sparc/log.h"
#include "sparc/KmerCountingListener.h"
#include "sparc/EdgeCountingClient.h"
#include "sparc/DBHelper.h"
using namespace std;
using namespace sparc;

struct Config {
	string inputpath;
	string outputpath;
	string scratch_dir;
	string mpi_hostname;
	string mpi_ipaddress;
	size_t min_shared_kmers;
	size_t max_degree;
	int port;
	int rank;
	int nprocs;
	DBHelper::DBTYPE dbtype;
	bool zip_output;
	std::vector<int> peers_ports;
	std::vector<std::string> peers_hosts;

	void print() {
		myinfo("config: max_degree=%ld", max_degree);
		myinfo("config: min_shared_kmers=%ld", min_shared_kmers);
		myinfo("config: zip_output=%d", zip_output ? 1 : 0);
		myinfo("config: inputpath=%s", outputpath.c_str());
		myinfo("config: outputpath=%s", outputpath.c_str());
		myinfo("config: scratch_dir=%s", scratch_dir.c_str());
		myinfo("config: dbpath=%s", get_dbpath().c_str());
		myinfo("config: dbtype=%s",
				dbtype == DBHelper::MEMORY_DB ?
						"memory" :
						(dbtype == DBHelper::LEVEL_DB ? "leveldb" : "rocksdb"));
		myinfo("config: #procs=%d", nprocs);
	}

	std::string get_dbpath() {

		char tmp[2048];
		if (zip_output) {
			sprintf(tmp, "%s/part_%d.txt.gz", outputpath.c_str(), rank);
		} else {
			sprintf(tmp, "%s/part_%d.txt", outputpath.c_str(), rank);
		}

		return tmp;
	}

	std::string get_my_output() {
		char tmp[2048];
		if (zip_output) {
			sprintf(tmp, "%s/kc_%d.txt.gz", outputpath.c_str(), rank);
		} else {
			sprintf(tmp, "%s/kc_%d.txt", outputpath.c_str(), rank);
		}
		return tmp;
	}

	int get_my_port() {
		return port + rank;
	}

};

string MPI_get_hostname() {
	char buf[2048];
	bzero(buf, 2048);
	int name_len;

	MPI_Get_processor_name(buf, &name_len);
	return buf;
}

void check_arg(argagg::parser_results &args, char *name) {
	if (!args[name]) {
		cerr << name << " is missing" << endl;
		exit(-1);
	}

}

int run(const std::vector<std::string> &input, Config &config);

int main(int argc, char **argv) {
	int rank, size;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	Config config;
	config.rank = rank;
	config.nprocs = size;
	config.mpi_hostname = MPI_get_hostname();
	config.mpi_ipaddress = get_ip_adderss(config.mpi_hostname);
	set_spdlog_pattern(config.mpi_hostname.c_str(), rank);

	myinfo("Welcome to Sparc!");

	argagg::parser argparser { {

	{ "help", { "-h", "--help" }, "shows this help message", 0 },

	{ "input", { "-i", "--input" },
			"input folder which contains read sequences", 1 },

	{ "port", { "-p", "--port" }, "port number", 1 },

	{ "scratch_dir", { "-s", "--scratch" },
			"scratch dir where to put temp data", 1 },

	{ "dbtype", { "--db" }, "dbtype (leveldb,rocksdb or default memdb)", 1 },

	{ "zip_output", { "-z", "--zip" }, "zip output files", 0 },

	{ "output", { "-o", "--output" }, "output folder", 1 },

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

	if (!args.pos.empty()) {
		std::cerr << "no positional argument is allowed" << endl;
		return EXIT_SUCCESS;
	}

	config.max_degree = args["max_degree"].as<int>(100);

	if (args["min_shared_kmers"]) {
		config.min_shared_kmers = args["min_shared_kmers"].as<int>();
	} else {
		config.min_shared_kmers = 2;
	}

	if (args["zip_output"]) {
		config.zip_output = true;
	} else {
		config.zip_output = false;
	}

	if (args["dbtype"]) {
		std::string dbtype = args["dbtype"].as<std::string>();
		if (dbtype == "memdb") {
			config.dbtype = DBHelper::MEMORY_DB;
		} else if (dbtype == "leveldb") {
			config.dbtype = DBHelper::LEVEL_DB;
		} else if (dbtype == "rocksdb") {
			config.dbtype = DBHelper::ROCKS_DB;
			cerr << "rocksdb was removed due to always coredump" << endl;
		} else {
			cerr << "Error, unknown dbtype:  " << dbtype << endl;
			return EXIT_FAILURE;
		}
	} else {
		config.dbtype = DBHelper::MEMORY_DB;
	}

	if (args["scratch_dir"]) {
		config.scratch_dir = args["scratch_dir"].as<string>();
	} else {
		config.scratch_dir = get_working_dir();
	}

	if (args["port"]) {
		config.port = args["port"].as<int>();
	} else {
		config.port = 7979;
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

	std::vector<std::string> input = list_dir(inputpath.c_str());
	if (input.size() == 0) {
		cerr << "Error, input dir is empty " << outputpath << endl;
		return EXIT_FAILURE;
	} else {
		if (rank == 0) {
			myinfo("#of inputs = %ld", input.size());
		}
	}
	std::vector<std::string> myinput;
	for (size_t i = 0; i < input.size(); i++) {
		if ((int) (i % size) == rank) {
			myinput.push_back(input.at(i));
		}
	}
	myinfo("#of my inputs = %ld", myinput.size());
	run(myinput, config);
	MPI_Finalize();

	return 0;
}

void get_peers_information(Config &config) {
	int rank = config.rank;

	for (int p = 0; p < config.nprocs; p++) { //p:  a peer
		int buf;
		if (rank == p) {
			buf = config.get_my_port();
		}

		/* everyone calls bcast, data is taken from root and ends up in everyone's buf */
		MPI_Bcast(&buf, 1, MPI_INT, p, MPI_COMM_WORLD);
		config.peers_ports.push_back(buf);
	}

	for (int p = 0; p < config.nprocs; p++) { //p:  a peer
		int LENGTH = 256;
		char buf[LENGTH];
		bzero(buf, LENGTH);

		if (rank == p) {
			strcpy(buf, config.mpi_ipaddress.c_str());
		}

		MPI_Bcast(&buf, LENGTH, MPI_CHAR, p, MPI_COMM_WORLD);
		config.peers_hosts.push_back(buf);
	}

}

int run(const std::vector<std::string> &input, Config &config) {
	if (config.rank == 0) {
		config.print();
	}
	get_peers_information(config);
	KmerCountingListener listener(config.mpi_ipaddress, config.get_my_port(),
			config.get_dbpath(), config.dbtype, false);
	myinfo("Starting listener");
	int status = listener.start();
	if (status != 0) {
		myerror("Start listener failed.");
		MPI_Abort( MPI_COMM_WORLD, -1);
	}

	//wait for all server is ready
	MPI_Barrier(MPI_COMM_WORLD);
	myinfo("Starting client");
	EdgeCountingClient client(config.peers_ports, config.peers_hosts);
	if (client.start() != 0) {
		myerror("Start client failed");
		MPI_Abort( MPI_COMM_WORLD, -1);
	}

	for (size_t i = 0; i < input.size(); i++) {
		myinfo("processing %s", input.at(i).c_str());
		client.process_krm_file(input.at(i), config.min_shared_kmers,
				config.max_degree);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	client.stop();
	myinfo("Total sent %ld kmers", client.get_n_sent());
	MPI_Barrier(MPI_COMM_WORLD);

	//cleanup listener and db
	listener.stop();
	myinfo("Total recved %ld kmers", listener.get_n_recv());
	listener.dumpdb(config.get_my_output(), ' ');
	listener.removedb();

	return 0;
}

