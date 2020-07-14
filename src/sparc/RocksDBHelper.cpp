/*
 * RocksDBHelper.cpp
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#include <fstream>
#include "rocksdb/cache.h"
#include "rocksdb/table.h"

#include "utils.h"
#include "log.h"
#include "RocksDBHelper.h"

using namespace sparc;
using namespace std;

RocksDBHelper::RocksDBHelper(const std::string &path, bool readonly) :
		DBHelper(readonly), dbpath(path), db(NULL) {

}

void RocksDBHelper::put(const std::string &key, uint32_t val) {
	db->Put(rocksdb::WriteOptions(), key, std::to_string(val));
}
void RocksDBHelper::put(const std::string &key, const std::string &val) {
	db->Put(rocksdb::WriteOptions(), key, val);
}

void RocksDBHelper::incr(const std::string &key, uint32_t n) {
	std::string value;
	rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, &value);
	if (s.ok()) {
		put(key, std::stoul(value) + n);
	} else {
		put(key, n);
	}
}

void RocksDBHelper::append(const std::string &key, uint32_t n) {
	append(key, std::to_string(n));
}

void RocksDBHelper::append(const std::string &key, const std::string &val) {
	std::string prev_value;
	rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, &prev_value);
	if (s.ok()) {
		put(key, prev_value + " " + val);
	} else {
		put(key, val);
	}

}

int RocksDBHelper::dump(const std::string &filepath, char sep) {
	int stat = 0;
	ofstream myfile;
	myfile.open(filepath);
	rocksdb::Iterator *it = db->NewIterator(rocksdb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		myfile << it->key().ToString() << sep << it->value().ToString() << endl;
	}
	stat = it->status().ok() ? 0 : -1;
	delete it;
	myfile.close();
	return stat;
}

int RocksDBHelper::create() {
	if (path_exists(dbpath)) {
		myinfo("remove path %s", dbpath.c_str());
		system((string("rm -fr ") + dbpath).c_str());
	}
	myinfo("Creating database at %s", dbpath.c_str());
	rocksdb::Options opts;
	rocksdb::DB *db = 0;
	opts.create_if_missing = true;

	opts.max_open_files = 300000;
	opts.write_buffer_size = 67108864;
	opts.max_write_buffer_number = 3;
	opts.target_file_size_base = 67108864;
	opts.compression = rocksdb::CompressionType::kLZ4Compression;

	rocksdb::BlockBasedTableOptions table_options;
	table_options.block_cache = rocksdb::NewLRUCache(2147483648);
	table_options.block_cache_compressed = rocksdb::NewLRUCache(524288000);
	opts.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
	rocksdb::Status status = rocksdb::DB::Open(opts, dbpath, &db);

	return status.ok() ? 0 : 1;
}

void RocksDBHelper::remove() {
	close();
	if (path_exists(dbpath)) {
		system((string("rm -fr ") + dbpath).c_str());
	}
}

void RocksDBHelper::close() {
	if (db) {
		delete db;
		db = NULL;
	}
}
RocksDBHelper::~RocksDBHelper() {
	close();
}
