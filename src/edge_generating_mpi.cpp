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
#include "sparc/EdgeGeneratingListener.h"
#include "sparc/EdgeGeneratingClient.h"
#include "sparc/DBHelper.h"
#include "sparc/config.h"
#include "mpihelper.h"
using namespace std;
using namespace sparc;

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
	int rank, size;

	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	if (provided == MPI_THREAD_SINGLE) {
		fprintf(stderr, "Could not initialize with thread support\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	Config config;
	config.program = argv[0];
	config.rank = rank;
	config.nprocs = size;
	config.mpi_hostname = MPI_get_hostname();
	config.mpi_ipaddress = get_ip_adderss(config.mpi_hostname);
	set_spdlog_pattern(config.mpi_hostname.c_str(), rank);

	if (rank == 0) {
		myinfo("Welcome to Sparc!");
	}

	argagg::parser argparser {
			{

			{ "help", { "-h", "--help" }, "shows this help message", 0 },

			{ "input", { "-i", "--input" },
					"input folder which contains read sequences", 1 },

			{ "port", { "-p", "--port" }, "port number", 1 },

			{ "n_iteration", { "-n" }, "number of iteration", 1 },

			{ "scratch_dir", { "-s", "--scratch" },
					"scratch dir where to put temp data", 1 },

			{ "dbtype", { "--db" }, "dbtype (leveldb,rocksdb or default memdb)",
					1 },

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
#ifdef BUILD_WITH_LEVELDB
			config.dbtype = DBHelper::LEVEL_DB;
#else
			cerr
					<< "was not compiled with leveldb suppot. Please cmake with BUILD_WITH_LEVELDB=ON"
					<< dbtype << endl;
			return EXIT_FAILURE;
#endif
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

	std::vector<std::string> myinput = get_my_files(config.inputpath, rank,
			size);
	myinfo("#of my inputs = %ld", myinput.size());
	run(myinput, config);
	MPI_Finalize();

	return 0;
}

void get_peers_information(int bucket, Config &config) {
	int rank = config.rank;
	config.peers_ports.clear();
	config.peers_hosts.clear();
	for (int p = 0; p < config.nprocs; p++) { //p:  a peer
		int buf;
		if (rank == p) {
			buf = config.get_my_port(bucket);
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

void reshuffle_rank(Config &config) {
	config.hash_rank_mapping.clear();
	int nproc = config.nprocs;
	int buf[nproc];
	if (config.rank == 0) {
		std::vector<int> v;
		for (int i = 0; i < nproc; i++) {
			v.push_back(i);
		}
		shuffle(v);
		for (int i = 0; i < nproc; i++) {
			buf[i] = v.at(i);
		}
	}

	MPI_Bcast(&buf, nproc, MPI_INT, 0, MPI_COMM_WORLD);
	for (int i = 0; i < nproc; i++) {
		config.hash_rank_mapping.push_back(buf[i]);
		if (config.rank == 0) {
			myinfo("hash %d will be sent to rank %d", i, buf[i]);
		}
	}
}

int run(int iteration, const std::vector<std::string> &input, Config &config) {

	get_peers_information(iteration, config);
	EdgeGeneratingListener listener(config.rank, config.nprocs,
			config.mpi_ipaddress, config.get_my_port(iteration),
			config.get_dbpath(), config.dbtype);
	myinfo("Starting listener");
	int status = listener.start();
	if (status != 0) {
		myerror("Start listener failed.");
		MPI_Abort( MPI_COMM_WORLD, -1);
	}

	//wait for all server is ready
	MPI_Barrier(MPI_COMM_WORLD);
	myinfo("Starting client");
	EdgeGeneratingClient client(config.rank, config.peers_ports,
			config.peers_hosts, config.hash_rank_mapping);
	if (client.start() != 0) {
		myerror("Start client failed");
		MPI_Abort( MPI_COMM_WORLD, -1);
	}

	for (size_t i = 0; i < input.size(); i++) {
		myinfo("processing %s", input.at(i).c_str());
		client.process_krm_file(iteration, config.n_iteration, input.at(i),
				config.min_shared_kmers, config.max_degree);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	client.stop();
	myinfo("Total sent %ld kmers", client.get_n_sent());
	MPI_Barrier(MPI_COMM_WORLD);

	//cleanup listener and db
	listener.stop();
	myinfo("Total recved %ld kmers", listener.get_n_recv());
	listener.dumpdb(config.get_my_output(iteration), ' ');
	listener.removedb();

	return 0;
}

int run(const std::vector<std::string> &input, Config &config) {
	if (config.rank == 0) {
		config.print();
	}
	reshuffle_rank(config);

	for (int i = 0; i < config.n_iteration; i++) {
		if (config.rank == 0) {
			myinfo("Starting iteration %d", i);
		}

		run(i, input, config);
		MPI_Barrier(MPI_COMM_WORLD);
	}
	return 0;

}
