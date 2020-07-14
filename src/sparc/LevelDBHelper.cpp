/*
 * LevelDBHelper.cpp
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#include <fstream>
#include "utils.h"
#include "log.h"
#include "LevelDBHelper.h"

using namespace sparc;
using namespace std;

LevelDBHelper::LevelDBHelper(const std::string &path) :
		DBHelper(false), dbpath(path), db(NULL) {

}

void LevelDBHelper::put(const std::string &key, uint32_t val) {
	db->Put(leveldb::WriteOptions(), key, std::to_string(val));
}
void LevelDBHelper::put(const std::string &key, const std::string &val) {
	db->Put(leveldb::WriteOptions(), key, val);
}

void LevelDBHelper::incr(const std::string &key, uint32_t n) {
	std::string value;
	leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
	if (s.ok()) {
		put(key, std::stoul(value) + n);
	} else if (s.IsNotFound()) {
		put(key, n);
	} else {
		myerror("[key=%s] %s", key.c_str(), s.ToString().c_str());
	}
}

void LevelDBHelper::append(const std::string &key, uint32_t n) {
	append(key, std::to_string(n));
}

void LevelDBHelper::append(const std::string &key, const std::string &val) {
	std::string prev_value;
	leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &prev_value);
	if (s.ok()) {
		put(key, prev_value + " " + val);
	} else {
		put(key, val);
	}

}

int LevelDBHelper::dump(const std::string &filepath, char sep) {
	int stat = 0;
	ofstream myfile;
	myfile.open(filepath);
	leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		myfile << it->key().ToString() << sep << it->value().ToString() << endl;
	}
	stat = it->status().ok() ? 0 : -1;
	delete it;
	myfile.close();
	return stat;
}

int LevelDBHelper::create() {
	if (path_exists(dbpath)) {
		myinfo("remove path %s", dbpath.c_str());
		system((string("rm -fr ") + dbpath).c_str());
	}
	myinfo("Creating database at %s", dbpath.c_str());
	leveldb::Options options;
	options.create_if_missing = true;
	options.max_open_files = 300000;
	options.write_buffer_size = 67108864;
	options.compression = leveldb::kSnappyCompression;
	leveldb::Status status = leveldb::DB::Open(options, dbpath, &db);
	return status.ok() ? 0 : 1;
}

void LevelDBHelper::remove() {
	close();
	if (path_exists(dbpath)) {
		system((string("rm -fr ") + dbpath).c_str());
	}
}

void LevelDBHelper::close() {
	if (db) {
		delete db;
		db = NULL;
	}
}
LevelDBHelper::~LevelDBHelper() {
	close();
}

