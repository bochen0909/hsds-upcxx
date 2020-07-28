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



#ifdef USE_MPICLIENT
#include "MPIClient.h"
class KmerCountingClient:public MPIClient {
#else
#include "ZMQClient.h"
class KmerCountingClient:public ZMQClient {
#endif
public:
	KmerCountingClient(int rank, const std::vector<int> &peers_ports,
			const std::vector<std::string> &peers_hosts,const std::vector<int> &hash_rank_mapping, bool do_appending);
	virtual ~KmerCountingClient();



	virtual int process_seq_file(const std::string &filepath, int kmer_length,
			bool without_canonical_kmer);
protected:
	inline virtual void map_line(const std::string &line, int kmer_length,
			bool without_canonical_kmer, std::vector<std::string> &kmers,
			std::vector<uint32_t> &nodeids);
	template<typename V> void send_kmers(const std::vector<std::string> &kmers,
			const std::vector<V> &nodeids);

protected:
	bool do_appending;
};

#endif /* SOURCE_DIRECTORY__SRC_SPARC_KMERCOUNTINGCLIENT_H_ */
