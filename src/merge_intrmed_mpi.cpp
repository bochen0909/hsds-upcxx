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
#include <chrono>
#include <mpi.h>

#include "argagg.hpp"
#include "gzstream.h"
#include "sparc/utils.h"
#include "kmer.h"
#include "sparc/log.h"
#include "sparc/MergeListener.h"
#include "sparc/MergeClient.h"
#include "sparc/DBHelper.h"
#include "mpihelper.h"
using namespace std;
using namespace sparc;

struct Config {
	bool append_merge;
	std::vector<string> inputpath;
	string outputpath;
	string scratch_dir;
	string mpi_hostname;
	string mpi_ipaddress;
	int num_bucket;
	int sep_pos;
	int port;
	int rank;
	int nprocs;
	char sep;
	DBHelper::DBTYPE dbtype;
	bool zip_output;
	std::vector<int> peers_ports;
	std::vector<int> hash_rank_mapping;
	std::vector<std::string> peers_hosts;

	void print() {
		myinfo("config: aggregate=%s", append_merge ? "append" : "sum");
		myinfo("config: num_bucket=%d", num_bucket ? 1 : 0);
		myinfo("config: sep_pos=%d", sep_pos);
		if (sep == '\t') {
			myinfo("config: sep=TAB");
		} else if (sep == ' ') {
			myinfo("config: sep=SPACE");
		} else {
			myinfo("config: sep=%c", sep);
		}
		myinfo("config: zip_output=%d", zip_output ? 1 : 0);
		for (size_t i = 0; i < inputpath.size(); i++) {
			myinfo("config: inputpath[%d]=%s", i, inputpath.at(i).c_str());
		}
		myinfo("config: outputpath=%s", outputpath.c_str());
		myinfo("config: scratch_dir=%s", scratch_dir.c_str());
		myinfo("config: dbtype=%s",
				dbtype == DBHelper::MEMORY_DB ?
						"memory" :
						(dbtype == DBHelper::LEVEL_DB ? "leveldb" : "rocksdb"));
		myinfo("config: #procs=%d", nprocs);
	}

	std::string get_dbpath(int h) {
		char tmp[2048];
		sprintf(tmp, "%s/part_h%d_r%d.db", scratch_dir.c_str(), h, rank);
		return tmp;
	}

	std::string get_my_output(int h) {
		char tmp[2048];
		if (zip_output) {
			sprintf(tmp, "%s/part_h%d_r%d.txt.gz", outputpath.c_str(), h, rank);
		} else {
			sprintf(tmp, "%s/part_h%d_r%d.txt", outputpath.c_str(), h, rank);
		}
		return tmp;
	}

	int get_my_port(int h) {
		return port + h * nprocs + rank;
		//return port +  rank;
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

int run(int bucket, const std::vector<std::string> &input, Config &config);
int run_bucket(int bucket, const std::vector<std::string> &input,
		Config &config);
void reshuffle_rank(Config &config);

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

	if (rank == 0) {
		myinfo("Welcome to Sparc!");
	}

	argagg::parser argparser {
			{

			{ "help", { "-h", "--help" }, "shows this help message", 0 },

			{ "port", { "-p", "--port" }, "port number", 1 },

			{ "scratch_dir", { "--scratch" },
					"scratch dir where to put temp data", 1 },

			{ "sep", { "--sep" }, "separate char between key and val", 1 },

					{ "sep_pos", { "--sp" },
							"which separator are used to split key value. default 1 (first one)",
							1 },

					{ "dbtype", { "--db" },
							"dbtype (leveldb,rocksdb or default memdb)", 1 },

					{ "zip_output", { "-z", "--zip" }, "zip output files", 0 },

					{ "output", { "-o", "--output" }, "output folder", 1 },

					{ "num_bucket", { "-n", "--num-bucket" },
							"process input as n bucket (save memory, trade space with time)",
							1 },

					{ "append_merge", { "--append-merge" },
							"merge as append instead of sum", 0 },

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

	config.num_bucket = args["num_bucket"].as<int>(1);
	if (args["sep"]) {
		string s = args["sep"].as<string>();
		if (s == "\\t") {
			config.sep = '\t';
		} else if (s.length() != 1) {
			myerror("separator can only be a a single char, but got '%s'",
					s.c_str());
			return EXIT_SUCCESS;
		} else {
			config.sep = s.at(0);
		}
	} else {
		config.sep = '\t';
	}

	config.sep_pos = args["sep_pos"].as<int>(1);
	config.append_merge = args["append_merge"];
	config.zip_output = args["zip_output"];

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
	reshuffle_rank(config);
	for (int b = 0; b < config.num_bucket; b++) {
		run_bucket(b, myinput, config);

	}
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

int run_bucket(int bucket, const std::vector<std::string> &input,
		Config &config) {
	if (config.rank == 0) {
		myinfo("Start running bucket %d", bucket);
	}
	myinfo("bucket %d: #of my inputs = %ld", bucket, input.size());
	MPI_Barrier(MPI_COMM_WORLD);
	return run(bucket, input, config);
	MPI_Barrier(MPI_COMM_WORLD);

}

int run(int bucket, const std::vector<std::string> &input, Config &config) {
	if (config.rank == 0) {
		config.print();
	}
	get_peers_information(bucket, config);
	MergeListener listener(config.mpi_ipaddress, config.get_my_port(bucket),
			config.get_dbpath(bucket), config.dbtype,
			config.append_merge/*similar as kr mapping*/);
	if (config.rank == 0) {
		myinfo("Starting listener");
	}
	int status = listener.start();
	if (status != 0) {
		myerror("Start listener failed.");
		MPI_Abort( MPI_COMM_WORLD, -1);
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));
	//wait for all server is ready
	MPI_Barrier(MPI_COMM_WORLD);
	if (config.rank == 0) {
		myinfo("Starting client");
	}
	MergeClient client(config.peers_ports, config.peers_hosts,
			config.hash_rank_mapping);
	if (client.start() != 0) {
		myerror("Start client failed");
		MPI_Abort( MPI_COMM_WORLD, -1);
	}

	for (size_t i = 0; i < input.size(); i++) {
		myinfo("processing %s", input.at(i).c_str());
		client.process_input_file(bucket, config.nprocs, input.at(i),
				config.sep, config.sep_pos);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	client.stop();
	myinfo("Total sent %ld records", client.get_n_sent());
	MPI_Barrier(MPI_COMM_WORLD);

	//cleanup listener and db
	listener.stop();
	myinfo("Total recved %ld records", listener.get_n_recv());
	listener.dumpdb(config.get_my_output(bucket));
	listener.removedb();

	return 0;
}

