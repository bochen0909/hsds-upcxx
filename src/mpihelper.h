/*
 * mpihelper.h
 *
 *  Created on: Jul 23, 2020
 *      Author:
 */

#ifndef SOURCE_DIRECTORY__SRC_MPIHELPER_H_
#define SOURCE_DIRECTORY__SRC_MPIHELPER_H_

#include "sparc/utils.h"
#include "sparc/log.h"
#include "kmer.h"

namespace sparc {
std::vector<std::string> get_all_files(
		const std::vector<std::string> &folders) {
	std::vector<std::string> input;
	for (size_t i = 0; i < folders.size(); i++) {
		std::vector<std::string> v = list_dir(folders.at(i).c_str());
		input.insert(input.end(), v.begin(), v.end());
	}
	sort(input.begin(), input.end());
	input.erase(unique(input.begin(), input.end()), input.end());
	shuffle(input);
	return input;
}

std::vector<std::vector<std::string>> get_my_files(
		const std::vector<std::string> &folders, int rank, int size,
		int n_bucket = 1) {
	std::vector<std::string> allfiles = get_all_files(folders);
	myinfo("#of all files: %ld", allfiles.size());
	std::vector<std::vector<std::string>> myinput;
	for (int i = 0; i < n_bucket; i++) {
		std::vector<std::string> thisinput;
		for (auto &file : allfiles) {
			uint32_t h = fnv_hash(file);
			if (h % size == rank && h % n_bucket == i) {
				thisinput.push_back(file);
			}
		}
		myinput.push_back(thisinput);
	}
	return myinput;
}

} //end namespace

#endif /* SOURCE_DIRECTORY__SRC_MPIHELPER_H_ */
