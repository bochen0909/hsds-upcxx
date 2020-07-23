/*
 * mpihelper.h
 *
 *  Created on: Jul 23, 2020
 *      Author:
 */

#ifndef SOURCE_DIRECTORY__SRC_MPIHELPER_H_
#define SOURCE_DIRECTORY__SRC_MPIHELPER_H_

#include "limits.h"
#include "sparc/utils.h"
#include "sparc/log.h"
#include "kmer.h"

namespace sparc {
bool check_all_peers_on_same_page(const std::vector<std::string> &files,
		int rank, int size) {

	{ //check #
		int n = 0;
		if (rank == 0) {
			n = (int) files.size();
		}
		MPI_Bcast(&n, size, MPI_INT, 0, MPI_COMM_WORLD);
		if (n != (int) files.size()) {
			myerror("#files is not equal to that of rank 0: r=%d, %ld<>%ld",
					rank, files.size(), n);
			return false;
		}
	}
	size_t N = files.size();
	for (int i = 0; i < N; i++) { //check files
		char buf[PATH_MAX];
		bzero(buf, PATH_MAX);
		if (rank == 0) {
			auto fpath = files.at(i);
			strcpy(buf, fpath.c_str());
		}
		MPI_Bcast(buf, PATH_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);

		std::string fpath(buf);
		if (std::find(files.begin(), files.end(), fpath) == files.end()) {
			myerror("%s cannot find locally for rank %d", buf, rank);
			return false;
		}
	}
	return true;
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

bool get_all_files_check(const std::vector<std::string> &folders,
		std::vector<std::string> &ret, int rank, int size) {
	std::vector<std::string> files;
	bool good = false;
	for (int i = 0; i < 3; i++) {
		ret.clear();
		get_all_files(folders, ret);
		good = check_all_peers_on_same_page(ret, rank, size);
		if (good) {
			break;
		} else {
			myerror("Check files failed for iteration %d", i);
		}
	}

	return good;

}

std::vector<std::vector<std::string>> get_my_files(
		const std::vector<std::string> &folders, int rank, int size,
		int n_bucket = 1) {
	std::vector<std::vector<std::string>> myinput;
	std::vector<std::string> allfiles;
	if (get_all_files_check(folders, allfiles, rank, size)) {
		myinfo("#of all files: %ld", allfiles.size());

		if (allfiles.empty()) {
			myerror("no input files found");
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		for (int i = 0; i < n_bucket; i++) {
			std::vector<std::string> thisinput;
			for (size_t j = 0; j < allfiles.size(); j++) {
				auto &file = allfiles.at(j);
				if (j % size == rank && j % n_bucket == i) {
					thisinput.push_back(file);
				}
			}
			myinput.push_back(thisinput);
		}

	} else {
		myerror("Check files failed for 3 retries");
		MPI_Abort(MPI_COMM_WORLD, -1);

	}
	return myinput;
}

vector<std::vector<std::string>> get_my_files(const std::string &folder,
		int rank, int size, int n_bucket) {
	return get_my_files( { folder }, rank, size, n_bucket);

}

std::vector<std::string> get_my_files(const std::string &folder, int rank,
		int size) {
	return get_my_files( { folder }, rank, size, 1).at(0);

}

std::vector<std::string> get_my_files(const std::vector<std::string> &folders,
		int rank, int size) {
	return get_my_files(folders, rank, size, 1).at(0);

}

} //end namespace

#endif /* SOURCE_DIRECTORY__SRC_MPIHELPER_H_ */
