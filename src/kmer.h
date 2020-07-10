#include <vector>
#include <string>

using namespace std;

unsigned long kmer_to_number(const string& kmer);

string number_to_kmer(unsigned long num, int k);

vector<string> generate_kmer(const string& seq, int k, char err_char,
		bool is_canonical);

vector<unsigned long> generate_kmer_number(const string& seq, int k,
		char err_char, bool is_canonical);

vector<string> generate_kmer_for_fastseq(const string& seq, int k,
		char err_char, bool is_canonical);

string random_generate_kmer(const string& seq, int k, bool is_canonical);

string canonical_kmer(const string& seq);

string reverse_complement(const string& seq);

void kmpSearch(const string& word, const string& text,
		std::vector<int>& positions);

std::vector<int> pigeonhole_knuth_morris_pratt_search(const string& longword,
		const string& text, int n_err_allowed);

int left_right_overlap(const string& left, const string& right,
		int min_over_lap, float err_rate);

int sequence_overlap(const string& seq1, const string& seq2, int min_over_lap,
		float err_rate);

void overlap_print(const string& left, const string& right, int pos);

std::vector<std::pair<uint32_t, uint32_t> > generate_edges(std::vector<uint32_t>& reads,
		int max_degree);

uint32_t fnv_hash(const std::string& str);

std::string ulong2base64(unsigned long n);

unsigned long base64toulong(const std::string& str);

string kmer_to_base64(const string &kmer);
