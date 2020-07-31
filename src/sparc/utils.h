/*
 * utils.h
 *
 *  Created on: May 16, 2020
 *      Author:
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>

namespace sparc {

std::string get_env(const std::string &var);

std::string get_working_dir();
std::string get_hostname();

bool endswith(std::string const &fullString, std::string const &ending);

inline std::string int2str(int i);

inline int str2int(const std::string str);

bool path_exists(const std::string &s);
bool file_exists(const char *filename);
bool dir_exists(const char *path);
int make_dir(const char *path);

uint32_t hash_string(const std::string &s);

std::vector<std::string> list_dir(const char *path);

// trim from start (in place)
inline void ltrim(std::string &s);

// trim from end (in place)
inline void rtrim(std::string &s);

// trim from both ends (in place)
void trim(std::string &s);

// trim from start (copying)
inline std::string ltrim_copy(std::string s);

// trim from end (copying)
inline std::string rtrim_copy(std::string s);

// trim from both ends (copying)
std::string trim_copy(std::string s);

void shuffle(std::vector<int>& v);

void shuffle(std::vector<std::string> &v);

std::vector<std::string> split(const std::string &source,
		const char *delimiter = " ", bool keepEmpty = false);

void split(std::vector<std::string> &results, const std::string &source,
		const char *delimiter = " ", bool keepEmpty = false);

std::vector<std::string> split_not_thread_safe(std::string str,
		std::string sep);

void split_not_thread_safe(std::vector<std::string> &arr, std::string str,
		std::string sep);

std::string get_ip_adderss();

std::string get_ip_adderss(const std::string &hostname);

}
#endif /* UTILS_H_ */
