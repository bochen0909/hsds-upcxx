/*
 *
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
#include "gzstream.h"
#include "sparc/utils.h"
#include "kmer.h"
#include "sparc/log.h"
#include "sparc/DBHelper.h"
#include "sparc/LPAState.h"
#include "sparc/LPAClient.h"
#include "mpihelper.h"
#include "sparc/LPAListener.h"
#include "sparc/config.h"
using namespace std;
using namespace sparc;

struct Config: public BaseConfig {
	int n_iteration;
	int smin;
	bool weighted;

	void print() {
		BaseConfig::print();
		myinfo("config: smin=%d", smin);
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
	config.program=argv[0];
	config.rank = rank;
	config.nprocs = size;
	config.mpi_hostname = MPI_get_hostname();
	config.mpi_ipaddress = get_ip_adderss(config.mpi_hostname);
	set_spdlog_pattern(config.mpi_hostname.c_str(), rank);

	if (rank == 0) {
		myinfo("Welcome to Sparc!");
	}

	argagg::parser argparser { {

	{ "help", { "-h", "--help" }, "shows this help message", 0 },

	{ "port", { "-p", "--port" }, "port number", 1 },

	{ "scratch_dir", { "--scratch" }, "scratch dir where to put temp data", 1 },

	{ "dbtype", { "--db" }, "dbtype (leveldb,rocksdb or default memdb)", 1 },

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
	if (args["dbtype"]) {
		mywarn("dbtype is useless here.");
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

	config.scratch_dir = args["scratch_dir"].as<string>(get_working_dir());
	config.port = args["port"].as<int>(7979);

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
	myinfo("#of my input: %ld", myinput.size());
	run(myinput, config);

	MPI_Finalize();

	return 0;
}

void get_peers_information(int bucket, Config &config) {
	int rank = config.rank;

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

int make_graph(int rank, const std::vector<std::string> &input,
		LPAClient &client, LPAListener &listener) {
	MPI_Barrier(MPI_COMM_WORLD);

	for (size_t i = 0; i < input.size(); i++) {
		myinfo("processing %s", input.at(i).c_str());
		client.process_input_file(input.at(i));
	}

	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0) {
		myinfo("Totally sent %ld edges", client.get_n_sent());
		myinfo("Totally recved %ld edges", listener.get_n_recv());
	}
	return 0;

}

bool check_finished(LPAClient &client, Config &config) {
	int n = config.nprocs;
	int32_t n_activate = client.getState().getNumActivateNode();

	int32_t all_activates[n];
	MPI_Allgather(&n_activate, 1, MPI_INT, all_activates, 1, MPI_INT,
	MPI_COMM_WORLD);

	size_t sum = 0;
	for (int i = 0; i < n; i++) {
		sum += all_activates[i];
	}
	if (config.rank == 0) {
		myinfo("# of activate node: %ld", sum);
	}
	return sum == 0;

}
int run(const std::vector<std::string> &input, Config &config) {

	int bucket = 0;
	reshuffle_rank(config);
	get_peers_information(0, config);
	MPI_Barrier(MPI_COMM_WORLD);

	if (config.rank == 0) {
		config.print();
	}

	LPAClient client(config.peers_ports, config.peers_hosts,
			config.hash_rank_mapping, config.smin, config.weighted,
			config.rank);

	MPI_Barrier(MPI_COMM_WORLD);

	LPAListener listener(config.rank, config.nprocs, config.mpi_ipaddress,
			config.get_my_port(bucket), config.get_dbpath(bucket),
			config.dbtype, client.getNode());
#ifdef USE_MPICLIENT
	client.setListener(&listener);
#endif
	if (config.rank == 0) {
		myinfo("Starting make-graph listener");
	}
	int status = listener.start();
	if (status != 0) {
		myerror("Start make-graph listener failed.");
		MPI_Abort( MPI_COMM_WORLD, -1);
	}
	MPI_Barrier(MPI_COMM_WORLD); //wait for server started

	if (config.rank == 0) {
		myinfo("Starting client");
	}

	if (client.start() != 0) {
		myerror("Start make-graph client failed");
		MPI_Abort( MPI_COMM_WORLD, -1);
	}
	MPI_Barrier(MPI_COMM_WORLD); //ensure client connected

	make_graph(config.rank, input, client, listener);
	MPI_Barrier(MPI_COMM_WORLD);
	client.getState().init();

#ifdef DEBUG
	client.getState().init_check();
#endif

	MPI_Barrier(MPI_COMM_WORLD);
	//ready to run
	for (int i = 0; i < config.n_iteration; i++) {
		client.run_iteration(i);
		MPI_Barrier(MPI_COMM_WORLD);
		if (check_finished(client, config)) {
			if (config.rank == 0) {
				myinfo("Finished at iteration %d", i);
			}
			break;
		}

		MPI_Barrier(MPI_COMM_WORLD);
	}

	client.stop();
	if (config.rank == 0) {
		myinfo("Totally sent %ld records", client.get_n_sent());
	}
	MPI_Barrier(MPI_COMM_WORLD);

	client.getState().dump(config.get_my_output(bucket));

	//cleanup listener and db
	listener.stop();
	if (config.rank == 0) {
		myinfo("Totally recved %ld records", listener.get_n_recv());
	}
	listener.removedb();

	return 0;

}

