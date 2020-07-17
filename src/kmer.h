#include <vector>
#include <string>

using namespace std;

unsigned long kmer_to_number(const string &kmer);

string number_to_kmer(unsigned long num, int k);

vector<string> generate_kmer(const string &seq, int k, char err_char,
		bool is_canonical);

vector<unsigned long> generate_kmer_number(const string &seq, int k,
		char err_char, bool is_canonical);

vector<string> generate_kmer_for_fastseq(const string &seq, int k,
		char err_char, bool is_canonical);

string random_generate_kmer(const string &seq, int k, bool is_canonical);

string canonical_kmer(const string &seq);

string reverse_complement(const string &seq);

std::vector<std::pair<uint32_t, uint32_t> > generate_edges(
		std::vector<uint32_t> &reads, size_t max_degree);

uint32_t fnv_hash(const std::string &str);

uint32_t fnv_hash(uint32_t n);

std::string ulong2base64(unsigned long n);

unsigned long base64toulong(const std::string &str);

string kmer_to_base64(const string &kmer);
