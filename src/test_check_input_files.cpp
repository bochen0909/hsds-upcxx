/*
 * edge_generating_mpi.cpp
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
#include "sparc/utils.h"
#include "sparc/log.h"
#include "mpihelper.h"
using namespace std;
using namespace sparc;

int main(int argc, char **argv) {
	int rank, size;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	argagg::parser argparser {
			{

			{ "help", { "-h", "--help" }, "shows this help message", 0 },

					{ "num_bucket", { "-n", "--num-bucket" },
							"process input as n bucket (save memory, trade space with time)",
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

	int num_bucket = args["num_bucket"].as<int>(1);

	std::vector<std::string> inputpath;
	if (args.pos.empty()) {
		std::cerr << "no input files are provided" << endl;
		return EXIT_SUCCESS;
	} else {
		inputpath = args.all_as<string>();
		bool b_error = false;
		for (size_t i = 0; i < inputpath.size(); i++) {
			if (!dir_exists(inputpath.at(i).c_str())) {
				cerr << "Error, input dir does not exists:  " << inputpath.at(i)
						<< endl;
				b_error = true;
			}
		}
		if (b_error) {
			return EXIT_FAILURE;
		}
	}
	std::vector<std::vector<std::string>> myinput = get_my_files(inputpath,
			rank, size, num_bucket);
	for (int i = 0; i < num_bucket; i++) {
		myinfo("rank %d, size %d, #folder %ld, #bucket %d, #input: %ld", rank,
				size, inputpath.size(), i, myinput.at(i).size());
	}

	MPI_Finalize();

}

