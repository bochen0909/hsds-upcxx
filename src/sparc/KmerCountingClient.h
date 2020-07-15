/*
 * KmerCountingClient.h
 *
 *  Created on: Jul 13, 2020
 *      Author: bo
 */

#ifndef SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGCLIENT_H_
#define SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGCLIENT_H_

#include <vector>
#include <string>
#include <zmqpp/zmqpp.hpp>

class KmerCountingClient {
public:
	KmerCountingClient(const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts, bool do_kr_mapping =
					false);
	virtual ~KmerCountingClient();

	virtual int start();
	virtual int stop();
	virtual uint64_t get_n_sent();

	virtual int process_seq_file(const std::string &filepath, int kmer_length,
			bool without_canonical_kmer);
protected:
	inline virtual void map_line(const std::string &line, int kmer_length,
			bool without_canonical_kmer, std::vector<std::string> &kmers,
			std::vector<uint32_t> &nodeids);
	virtual void send_kmers(const std::vector<std::string> &kmers,
			const std::vector<uint32_t> &nodeids);
protected:
	std::vector<int> peers_ports;
	std::vector<std::string> peers_hosts;
	std::vector<zmqpp::socket*> peers;
	zmqpp::context *context;
	uint64_t n_send;
	bool do_kr_mapping;
};

#endif /* SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGCLIENT_H_ */
