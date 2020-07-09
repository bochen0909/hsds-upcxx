/*
 * utils.h
 *
 *  Created on: May 16, 2020
 *      Author:
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstring>
#include <set>
#include <map>
#include <algorithm>
#include <cctype>
#include <locale>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

namespace sparc {
static inline std::string int2str(int i) {
	return std::to_string(i);
}

static inline int str2int(const std::string str) {
	return std::stoi(str);
}

bool file_exists(const char *filename) {
	std::ifstream f(filename);
	if (f.good()) {
		f.close();
		return true;
	} else {
		f.close();
		return false;
	}
}

bool dir_exists(const char *path) {
	struct stat info;

	if (stat(path, &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;
	else
		return false;
}

int make_dir(const char *path) {
	int status = mkdir(path, S_IRWXU | S_IRUSR | S_IXUSR);
	return status;
}

std::vector<std::string> list_dir(const char *path) {
	std::vector < std::string > ret;
	std::string strpath = path;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(path)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			std::string dirname = ent->d_name;
			if (dirname != "." && dirname != "..") {
				ret.push_back(strpath + "/" + dirname);
			}
		}
		closedir(dir);
	} else {
	}

	return ret;
}
// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
	ltrim(s);
	return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
	rtrim(s);
	return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
	trim(s);
	return s;
}

std::vector<std::string> split(std::string str, std::string sep) {
	char *cstr = const_cast<char*>(str.c_str());
	char *current;
	std::vector < std::string > arr;
	current = strtok(cstr, sep.c_str());
	while (current != NULL) {
		arr.push_back(current);
		current = strtok(NULL, sep.c_str());
	}
	return arr;
}


}
#endif /* UTILS_H_ */
