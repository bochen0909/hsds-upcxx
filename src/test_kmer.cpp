// Let Catch provide main():
#define CATCH_CONFIG_MAIN


#include <iostream>
#include "catch.hpp"
#include "kmer.h"

TEST_CASE( "kmer and number ", "[kmer]" ) {
	unsigned long n;
	string seq = "ATCG";
	n = kmer_to_number(seq);
	REQUIRE(n == 147);
	REQUIRE(seq == number_to_kmer(n, seq.size()));

	seq = "ATCGATC";
	n = kmer_to_number(seq);
	REQUIRE(n == 9444);
	REQUIRE(seq == number_to_kmer(n, seq.size()));

	seq = "ATCGACGTAACCACTGGTAC";
	n = kmer_to_number(seq);
	REQUIRE(n == 633736300504);
	REQUIRE(seq == number_to_kmer(n, seq.size()));

}

TEST_CASE( "test with canonical kmer ", "[kmer]" ) {

	string s1 = "GCAAAGAAAACGCTGGAAAACAGCGTGCTGG";
	string s2 = "CCAGCACGCTGTTTTCCAGCGTTTTCTTTGC";

	REQUIRE(s2 == reverse_complement(s1));
	REQUIRE(s1 == reverse_complement(s2));

	vector<string> v;

	v = generate_kmer(s1, 31, 'N', true);
	REQUIRE(v.size() == 1);
	REQUIRE(v[0] == s2);

	v = generate_kmer(s2, 31, 'N', true);
	REQUIRE(v.size() == 1);
	REQUIRE(v[0] == s2);
}

TEST_CASE( "test with small seq ", "[kmer]" ) {
	string seq = "ATCG";
	int k = 4;
	vector<string> kmers = generate_kmer(seq, k, 'N', true);
	REQUIRE(1 == kmers.size());

	seq = "ATCG";
	k = 5;
	kmers = generate_kmer(seq, k, 'N', true);
	REQUIRE(0 == kmers.size());

	// "Kmer generator" should "generate 4 kmers since ATC is duplicate"
	seq = "ATCGATC";
	k = 3;
	kmers = generate_kmer(seq, k, 'N', false);
	REQUIRE(4 == kmers.size());

	// "Kmer generator" should "generate 2 kmers
	seq = "ATCGATC";
	k = 3;
	kmers = generate_kmer(seq, k, 'N', true);
	REQUIRE(2 == kmers.size());

	seq = "ATCGACGTNACNACTGGTAC";
	k = 5;
	kmers = generate_kmer(seq, k, 'N', true);
	REQUIRE(8 == kmers.size());

}

TEST_CASE( "test kmer gen for numbers", "[kmer]" ) {

	string seq = "ATCGACGTNACNACTGGTAC";
	int k = 5;
	vector<string> kmers = generate_kmer(seq, k, 'N', true);
	REQUIRE(8 == kmers.size());
	vector<unsigned long> numbers = generate_kmer_number(seq, k, 'N', true);
	REQUIRE(kmers.size() == numbers.size());
	for (size_t i = 0; i < kmers.size(); i++) {
		REQUIRE(numbers.at(i) == kmer_to_number(kmers.at(i)));
		cout << numbers.at(i) << ", ";
	}
	cout << endl;

}

TEST_CASE( "test kmer gen for fastseq", "[kmer]" ) {
	string seq = "ATCG";
	int k = 4;
	vector<string> kmers = generate_kmer_for_fastseq(seq, k, 'N', true);
	REQUIRE(1 == kmers.size());
	cout << kmers[0] << endl;

	seq = "ATCG";
	k = 5;
	kmers = generate_kmer_for_fastseq(seq, k, 'N', true);
	REQUIRE(0 == kmers.size());

	seq = "ATCGATC";
	k = 3;
	kmers = generate_kmer_for_fastseq(seq, k, 'N', false);
	REQUIRE(5 == kmers.size());

	seq = "ATCGATC";
	k = 3;
	kmers = generate_kmer_for_fastseq(seq, k, 'N', true);
	REQUIRE(5 == kmers.size());

	seq = "ATCGACGTNACNACTGGTAC";
	k = 5;
	kmers = generate_kmer_for_fastseq(seq, k, 'N', true);
	REQUIRE(8 - 5 + 1 + 8 - 5 + 1 == kmers.size());

}

TEST_CASE( "test with bigger seq ", "[kmer]" ) {

	//"Kmer generator" should "work as expected"
	int k = 31;
	vector<string> kmers1 =
			{ "AGGGCAACAGAAGATGATAACAGAAAGGCAT",
					"GGGCAACAGAAGATGATAACAGAAAGGCATT",
					"GGCAACAGAAGATGATAACAGAAAGGCATTT",
					"GCAACAGAAGATGATAACAGAAAGGCATTTT",
					"CAACAGAAGATGATAACAGAAAGGCATTTTC",
					"AACAGAAGATGATAACAGAAAGGCATTTTCC",
					"ACAGAAGATGATAACAGAAAGGCATTTTCCA",
					"CAGAAGATGATAACAGAAAGGCATTTTCCAT",
					"AGAAGATGATAACAGAAAGGCATTTTCCATG",
					"GAAGATGATAACAGAAAGGCATTTTCCATGC",
					"AAGATGATAACAGAAAGGCATTTTCCATGCC",
					"AGATGATAACAGAAAGGCATTTTCCATGCCT",
					"GATGATAACAGAAAGGCATTTTCCATGCCTA",
					"ATGATAACAGAAAGGCATTTTCCATGCCTAT",
					"TGATAACAGAAAGGCATTTTCCATGCCTATA",
					"GATAACAGAAAGGCATTTTCCATGCCTATAT",
					"ATAACAGAAAGGCATTTTCCATGCCTATATT",
					"TAACAGAAAGGCATTTTCCATGCCTATATTC",
					"AACAGAAAGGCATTTTCCATGCCTATATTCG",
					"ACAGAAAGGCATTTTCCATGCCTATATTCGG",
					"CAGAAAGGCATTTTCCATGCCTATATTCGGT",
					"AGAAAGGCATTTTCCATGCCTATATTCGGTC",
					"GAAAGGCATTTTCCATGCCTATATTCGGTCG",
					"AAAGGCATTTTCCATGCCTATATTCGGTCGT",
					"AAGGCATTTTCCATGCCTATATTCGGTCGTT",
					"AGGCATTTTCCATGCCTATATTCGGTCGTTT",
					"GGCATTTTCCATGCCTATATTCGGTCGTTTT",
					"GCATTTTCCATGCCTATATTCGGTCGTTTTA",
					"CATTTTCCATGCCTATATTCGGTCGTTTTAC",
					"ATTTTCCATGCCTATATTCGGTCGTTTTACC",
					"TTTTCCATGCCTATATTCGGTCGTTTTACCC",
					"TTTCCATGCCTATATTCGGTCGTTTTACCCA",
					"TTCCATGCCTATATTCGGTCGTTTTACCCAG",
					"TCCATGCCTATATTCGGTCGTTTTACCCAGA",
					"CCATGCCTATATTCGGTCGTTTTACCCAGAG",
					"CATGCCTATATTCGGTCGTTTTACCCAGAGA",
					"ATGCCTATATTCGGTCGTTTTACCCAGAGAG",
					"TGCCTATATTCGGTCGTTTTACCCAGAGAGC",
					"GCCTATATTCGGTCGTTTTACCCAGAGAGCC",
					"CCTATATTCGGTCGTTTTACCCAGAGAGCCC",
					"CTATATTCGGTCGTTTTACCCAGAGAGCCCA",
					"TATATTCGGTCGTTTTACCCAGAGAGCCCAG",
					"ATATTCGGTCGTTTTACCCAGAGAGCCCAGC",
					"TATTCGGTCGTTTTACCCAGAGAGCCCAGCA",
					"ATTCGGTCGTTTTACCCAGAGAGCCCAGCAG",
					"TTCGGTCGTTTTACCCAGAGAGCCCAGCAGA",
					"TCGGTCGTTTTACCCAGAGAGCCCAGCAGAC",
					"CGGTCGTTTTACCCAGAGAGCCCAGCAGACA",
					"GGTCGTTTTACCCAGAGAGCCCAGCAGACAC",
					"GTCGTTTTACCCAGAGAGCCCAGCAGACACT",
					"TCGTTTTACCCAGAGAGCCCAGCAGACACTG",
					"CGTTTTACCCAGAGAGCCCAGCAGACACTGA",
					"GTTTTACCCAGAGAGCCCAGCAGACACTGAT",
					"TTTTACCCAGAGAGCCCAGCAGACACTGATG",
					"TTTACCCAGAGAGCCCAGCAGACACTGATGC",
					"TTACCCAGAGAGCCCAGCAGACACTGATGCT",
					"TACCCAGAGAGCCCAGCAGACACTGATGCTG",
					"ACCCAGAGAGCCCAGCAGACACTGATGCTGG",
					"CCCAGAGAGCCCAGCAGACACTGATGCTGGC",
					"CCAGAGAGCCCAGCAGACACTGATGCTGGCC",
					"CAGAGAGCCCAGCAGACACTGATGCTGGCCC",
					"AGAGAGCCCAGCAGACACTGATGCTGGCCCA",
					"GAGAGCCCAGCAGACACTGATGCTGGCCCAG",
					"AGAGCCCAGCAGACACTGATGCTGGCCCAGC",
					"GAGCCCAGCAGACACTGATGCTGGCCCAGCG",
					"AGCCCAGCAGACACTGATGCTGGCCCAGCGG",
					"GCCCAGCAGACACTGATGCTGGCCCAGCGGA",
					"CCCAGCAGACACTGATGCTGGCCCAGCGGAT",
					"CCAGCAGACACTGATGCTGGCCCAGCGGATC",
					"CAGCAGACACTGATGCTGGCCCAGCGGATCG",
					"AGCAGACACTGATGCTGGCCCAGCGGATCGC",
					"GCGATCCGCTGGGCCAGCATCAGTGTCTGCT",
					"CGATCCGCTGGGCCAGCATCAGTGTCTGCTG",
					"GATCCGCTGGGCCAGCATCAGTGTCTGCTGG",
					"ATCCGCTGGGCCAGCATCAGTGTCTGCTGGG",
					"TCCGCTGGGCCAGCATCAGTGTCTGCTGGGC",
					"CCGCTGGGCCAGCATCAGTGTCTGCTGGGCT",
					"CGCTGGGCCAGCATCAGTGTCTGCTGGGCTC",
					"GCTGGGCCAGCATCAGTGTCTGCTGGGCTCT",
					"CTGGGCCAGCATCAGTGTCTGCTGGGCTCTC",
					"TGGGCCAGCATCAGTGTCTGCTGGGCTCTCT",
					"GGGCCAGCATCAGTGTCTGCTGGGCTCTCTG",
					"GGCCAGCATCAGTGTCTGCTGGGCTCTCTGG",
					"GCCAGCATCAGTGTCTGCTGGGCTCTCTGGG",
					"CCAGCATCAGTGTCTGCTGGGCTCTCTGGGT",
					"CAGCATCAGTGTCTGCTGGGCTCTCTGGGTA",
					"AGCATCAGTGTCTGCTGGGCTCTCTGGGTAA",
					"GCATCAGTGTCTGCTGGGCTCTCTGGGTAAA",
					"CATCAGTGTCTGCTGGGCTCTCTGGGTAAAA",
					"ATCAGTGTCTGCTGGGCTCTCTGGGTAAAAC",
					"TCAGTGTCTGCTGGGCTCTCTGGGTAAAACG",
					"CAGTGTCTGCTGGGCTCTCTGGGTAAAACGA",
					"AGTGTCTGCTGGGCTCTCTGGGTAAAACGAC",
					"GTGTCTGCTGGGCTCTCTGGGTAAAACGACC",
					"TGTCTGCTGGGCTCTCTGGGTAAAACGACCG",
					"GTCTGCTGGGCTCTCTGGGTAAAACGACCGA",
					"TCTGCTGGGCTCTCTGGGTAAAACGACCGAA",
					"CTGCTGGGCTCTCTGGGTAAAACGACCGAAT",
					"TGCTGGGCTCTCTGGGTAAAACGACCGAATA",
					"GCTGGGCTCTCTGGGTAAAACGACCGAATAT",
					"CTGGGCTCTCTGGGTAAAACGACCGAATATA",
					"TGGGCTCTCTGGGTAAAACGACCGAATATAG",
					"GGGCTCTCTGGGTAAAACGACCGAATATAGG",
					"GGCTCTCTGGGTAAAACGACCGAATATAGGC",
					"GCTCTCTGGGTAAAACGACCGAATATAGGCA",
					"CTCTCTGGGTAAAACGACCGAATATAGGCAT",
					"TCTCTGGGTAAAACGACCGAATATAGGCATG",
					"CTCTGGGTAAAACGACCGAATATAGGCATGG",
					"TCTGGGTAAAACGACCGAATATAGGCATGGA",
					"CTGGGTAAAACGACCGAATATAGGCATGGAA",
					"TGGGTAAAACGACCGAATATAGGCATGGAAA",
					"GGGTAAAACGACCGAATATAGGCATGGAAAA",
					"GGTAAAACGACCGAATATAGGCATGGAAAAT",
					"GTAAAACGACCGAATATAGGCATGGAAAATG",
					"TAAAACGACCGAATATAGGCATGGAAAATGC",
					"AAAACGACCGAATATAGGCATGGAAAATGCC",
					"AAACGACCGAATATAGGCATGGAAAATGCCT",
					"AACGACCGAATATAGGCATGGAAAATGCCTT",
					"ACGACCGAATATAGGCATGGAAAATGCCTTT",
					"CGACCGAATATAGGCATGGAAAATGCCTTTC",
					"GACCGAATATAGGCATGGAAAATGCCTTTCT",
					"ACCGAATATAGGCATGGAAAATGCCTTTCTG",
					"CCGAATATAGGCATGGAAAATGCCTTTCTGT",
					"CGAATATAGGCATGGAAAATGCCTTTCTGTT",
					"GAATATAGGCATGGAAAATGCCTTTCTGTTA",
					"AATATAGGCATGGAAAATGCCTTTCTGTTAT",
					"ATATAGGCATGGAAAATGCCTTTCTGTTATC",
					"TATAGGCATGGAAAATGCCTTTCTGTTATCA",
					"ATAGGCATGGAAAATGCCTTTCTGTTATCAT",
					"TAGGCATGGAAAATGCCTTTCTGTTATCATC",
					"AGGCATGGAAAATGCCTTTCTGTTATCATCT",
					"GGCATGGAAAATGCCTTTCTGTTATCATCTT",
					"GCATGGAAAATGCCTTTCTGTTATCATCTTC",
					"CATGGAAAATGCCTTTCTGTTATCATCTTCT",
					"ATGGAAAATGCCTTTCTGTTATCATCTTCTG",
					"TGGAAAATGCCTTTCTGTTATCATCTTCTGT",
					"GGAAAATGCCTTTCTGTTATCATCTTCTGTT",
					"GAAAATGCCTTTCTGTTATCATCTTCTGTTG",
					"AAAATGCCTTTCTGTTATCATCTTCTGTTGC",
					"AAATGCCTTTCTGTTATCATCTTCTGTTGCC",
					"AATGCCTTTCTGTTATCATCTTCTGTTGCCC",
					"ATGCCTTTCTGTTATCATCTTCTGTTGCCCT",
					"TCTGTCCCAGCTTATGGGATTCCAGCACGCT",
					"CTGTCCCAGCTTATGGGATTCCAGCACGCTG",
					"TGTCCCAGCTTATGGGATTCCAGCACGCTGT",
					"GTCCCAGCTTATGGGATTCCAGCACGCTGTT",
					"TCCCAGCTTATGGGATTCCAGCACGCTGTTT",
					"CCCAGCTTATGGGATTCCAGCACGCTGTTTT",
					"CCAGCTTATGGGATTCCAGCACGCTGTTTTC",
					"CAGCTTATGGGATTCCAGCACGCTGTTTTCC",
					"AGCTTATGGGATTCCAGCACGCTGTTTTCCA",
					"GCTTATGGGATTCCAGCACGCTGTTTTCCAG",
					"CTTATGGGATTCCAGCACGCTGTTTTCCAGC",
					"TTATGGGATTCCAGCACGCTGTTTTCCAGCG",
					"TATGGGATTCCAGCACGCTGTTTTCCAGCGT",
					"ATGGGATTCCAGCACGCTGTTTTCCAGCGTT",
					"TGGGATTCCAGCACGCTGTTTTCCAGCGTTT",
					"GGGATTCCAGCACGCTGTTTTCCAGCGTTTT",
					"GGATTCCAGCACGCTGTTTTCCAGCGTTTTC",
					"GATTCCAGCACGCTGTTTTCCAGCGTTTTCT",
					"ATTCCAGCACGCTGTTTTCCAGCGTTTTCTT",
					"TTCCAGCACGCTGTTTTCCAGCGTTTTCTTT",
					"TCCAGCACGCTGTTTTCCAGCGTTTTCTTTG",
					"CCAGCACGCTGTTTTCCAGCGTTTTCTTTGC",
					"CAGCACGCTGTTTTCCAGCGTTTTCTTTGCC",
					"AGCACGCTGTTTTCCAGCGTTTTCTTTGCCC",
					"GCACGCTGTTTTCCAGCGTTTTCTTTGCCCT",
					"CACGCTGTTTTCCAGCGTTTTCTTTGCCCTG",
					"ACGCTGTTTTCCAGCGTTTTCTTTGCCCTGG",
					"CGCTGTTTTCCAGCGTTTTCTTTGCCCTGGG",
					"GCTGTTTTCCAGCGTTTTCTTTGCCCTGGGG",
					"CTGTTTTCCAGCGTTTTCTTTGCCCTGGGGC",
					"TGTTTTCCAGCGTTTTCTTTGCCCTGGGGCT",
					"GTTTTCCAGCGTTTTCTTTGCCCTGGGGCTC",
					"TTTTCCAGCGTTTTCTTTGCCCTGGGGCTCA",
					"TTTCCAGCGTTTTCTTTGCCCTGGGGCTCAG",
					"TTCCAGCGTTTTCTTTGCCCTGGGGCTCAGC",
					"TCCAGCGTTTTCTTTGCCCTGGGGCTCAGCT",
					"CCAGCGTTTTCTTTGCCCTGGGGCTCAGCTC",
					"CAGCGTTTTCTTTGCCCTGGGGCTCAGCTCG",
					"AGCGTTTTCTTTGCCCTGGGGCTCAGCTCGA",
					"GCGTTTTCTTTGCCCTGGGGCTCAGCTCGAT",
					"CGTTTTCTTTGCCCTGGGGCTCAGCTCGATC",
					"GTTTTCTTTGCCCTGGGGCTCAGCTCGATCC",
					"TTTTCTTTGCCCTGGGGCTCAGCTCGATCCG",
					"TTTCTTTGCCCTGGGGCTCAGCTCGATCCGG",
					"TTCTTTGCCCTGGGGCTCAGCTCGATCCGGT",
					"TCTTTGCCCTGGGGCTCAGCTCGATCCGGTT",
					"CTTTGCCCTGGGGCTCAGCTCGATCCGGTTT",
					"TTTGCCCTGGGGCTCAGCTCGATCCGGTTTC",
					"TTGCCCTGGGGCTCAGCTCGATCCGGTTTCC",
					"TGCCCTGGGGCTCAGCTCGATCCGGTTTCCT",
					"GCCCTGGGGCTCAGCTCGATCCGGTTTCCTC",
					"CCCTGGGGCTCAGCTCGATCCGGTTTCCTCT",
					"CCTGGGGCTCAGCTCGATCCGGTTTCCTCTG",
					"CTGGGGCTCAGCTCGATCCGGTTTCCTCTGG",
					"TGGGGCTCAGCTCGATCCGGTTTCCTCTGGC",
					"GGGGCTCAGCTCGATCCGGTTTCCTCTGGCC",
					"GGGCTCAGCTCGATCCGGTTTCCTCTGGCCT",
					"GGCTCAGCTCGATCCGGTTTCCTCTGGCCTC",
					"GCTCAGCTCGATCCGGTTTCCTCTGGCCTCT",
					"CTCAGCTCGATCCGGTTTCCTCTGGCCTCTT",
					"TCAGCTCGATCCGGTTTCCTCTGGCCTCTTC",
					"CAGCTCGATCCGGTTTCCTCTGGCCTCTTCC",
					"AGCTCGATCCGGTTTCCTCTGGCCTCTTCCG",
					"GCTCGATCCGGTTTCCTCTGGCCTCTTCCGG",
					"CTCGATCCGGTTTCCTCTGGCCTCTTCCGGA",
					"TCGATCCGGTTTCCTCTGGCCTCTTCCGGAT",
					"CGATCCGGTTTCCTCTGGCCTCTTCCGGATC",
					"GATCCGGTTTCCTCTGGCCTCTTCCGGATCA",
					"ATCCGGTTTCCTCTGGCCTCTTCCGGATCAC",
					"TCCGGTTTCCTCTGGCCTCTTCCGGATCACC",
					"CCGGTTTCCTCTGGCCTCTTCCGGATCACCG",
					"CGGTGATCCGGAAGAGGCCAGAGGAAACCGG",
					"GGTGATCCGGAAGAGGCCAGAGGAAACCGGA",
					"GTGATCCGGAAGAGGCCAGAGGAAACCGGAT",
					"TGATCCGGAAGAGGCCAGAGGAAACCGGATC",
					"GATCCGGAAGAGGCCAGAGGAAACCGGATCG",
					"ATCCGGAAGAGGCCAGAGGAAACCGGATCGA",
					"TCCGGAAGAGGCCAGAGGAAACCGGATCGAG",
					"CCGGAAGAGGCCAGAGGAAACCGGATCGAGC",
					"CGGAAGAGGCCAGAGGAAACCGGATCGAGCT",
					"GGAAGAGGCCAGAGGAAACCGGATCGAGCTG",
					"GAAGAGGCCAGAGGAAACCGGATCGAGCTGA",
					"AAGAGGCCAGAGGAAACCGGATCGAGCTGAG",
					"AGAGGCCAGAGGAAACCGGATCGAGCTGAGC",
					"GAGGCCAGAGGAAACCGGATCGAGCTGAGCC",
					"AGGCCAGAGGAAACCGGATCGAGCTGAGCCC",
					"GGCCAGAGGAAACCGGATCGAGCTGAGCCCC",
					"GCCAGAGGAAACCGGATCGAGCTGAGCCCCA",
					"CCAGAGGAAACCGGATCGAGCTGAGCCCCAG",
					"CAGAGGAAACCGGATCGAGCTGAGCCCCAGG",
					"AGAGGAAACCGGATCGAGCTGAGCCCCAGGG",
					"GAGGAAACCGGATCGAGCTGAGCCCCAGGGC",
					"AGGAAACCGGATCGAGCTGAGCCCCAGGGCA",
					"GGAAACCGGATCGAGCTGAGCCCCAGGGCAA",
					"GAAACCGGATCGAGCTGAGCCCCAGGGCAAA",
					"AAACCGGATCGAGCTGAGCCCCAGGGCAAAG",
					"AACCGGATCGAGCTGAGCCCCAGGGCAAAGA",
					"ACCGGATCGAGCTGAGCCCCAGGGCAAAGAA",
					"CCGGATCGAGCTGAGCCCCAGGGCAAAGAAA",
					"CGGATCGAGCTGAGCCCCAGGGCAAAGAAAA",
					"GGATCGAGCTGAGCCCCAGGGCAAAGAAAAC",
					"GATCGAGCTGAGCCCCAGGGCAAAGAAAACG",
					"ATCGAGCTGAGCCCCAGGGCAAAGAAAACGC",
					"TCGAGCTGAGCCCCAGGGCAAAGAAAACGCT",
					"CGAGCTGAGCCCCAGGGCAAAGAAAACGCTG",
					"GAGCTGAGCCCCAGGGCAAAGAAAACGCTGG",
					"AGCTGAGCCCCAGGGCAAAGAAAACGCTGGA",
					"GCTGAGCCCCAGGGCAAAGAAAACGCTGGAA",
					"CTGAGCCCCAGGGCAAAGAAAACGCTGGAAA",
					"TGAGCCCCAGGGCAAAGAAAACGCTGGAAAA",
					"GAGCCCCAGGGCAAAGAAAACGCTGGAAAAC",
					"AGCCCCAGGGCAAAGAAAACGCTGGAAAACA",
					"GCCCCAGGGCAAAGAAAACGCTGGAAAACAG",
					"CCCCAGGGCAAAGAAAACGCTGGAAAACAGC",
					"CCCAGGGCAAAGAAAACGCTGGAAAACAGCG",
					"CCAGGGCAAAGAAAACGCTGGAAAACAGCGT",
					"CAGGGCAAAGAAAACGCTGGAAAACAGCGTG",
					"AGGGCAAAGAAAACGCTGGAAAACAGCGTGC",
					"GGGCAAAGAAAACGCTGGAAAACAGCGTGCT",
					"GGCAAAGAAAACGCTGGAAAACAGCGTGCTG",
					"GCAAAGAAAACGCTGGAAAACAGCGTGCTGG",
					"CAAAGAAAACGCTGGAAAACAGCGTGCTGGA",
					"AAAGAAAACGCTGGAAAACAGCGTGCTGGAA",
					"AAGAAAACGCTGGAAAACAGCGTGCTGGAAT",
					"AGAAAACGCTGGAAAACAGCGTGCTGGAATC",
					"GAAAACGCTGGAAAACAGCGTGCTGGAATCC",
					"AAAACGCTGGAAAACAGCGTGCTGGAATCCC",
					"AAACGCTGGAAAACAGCGTGCTGGAATCCCA",
					"AACGCTGGAAAACAGCGTGCTGGAATCCCAT",
					"ACGCTGGAAAACAGCGTGCTGGAATCCCATA",
					"CGCTGGAAAACAGCGTGCTGGAATCCCATAA",
					"GCTGGAAAACAGCGTGCTGGAATCCCATAAG",
					"CTGGAAAACAGCGTGCTGGAATCCCATAAGC",
					"TGGAAAACAGCGTGCTGGAATCCCATAAGCT",
					"GGAAAACAGCGTGCTGGAATCCCATAAGCTG",
					"GAAAACAGCGTGCTGGAATCCCATAAGCTGG",
					"AAAACAGCGTGCTGGAATCCCATAAGCTGGG",
					"AAACAGCGTGCTGGAATCCCATAAGCTGGGA",
					"AACAGCGTGCTGGAATCCCATAAGCTGGGAC",
					"ACAGCGTGCTGGAATCCCATAAGCTGGGACA",
					"CAGCGTGCTGGAATCCCATAAGCTGGGACAG",
					"AGCGTGCTGGAATCCCATAAGCTGGGACAGA" };
	std::sort(kmers1.begin(), kmers1.end());

	string seq =
			"AGGGCAACAGAAGATGATAACAGAAAGGCATTTTCCATGCCTATATTCGGTCGTTTTACCCAGAGAGCCCAGCAGACACTGATGCTGGCCCAGCGGATCGCNTCTGTCCCAGCTTATGGGATTCCAGCACGCTGTTTTCCAGCGTTTTCTTTGCCCTGGGGCTCAGCTCGATCCGGTTTCCTCTGGCCTCTTCCGGATCACCG";

	SECTION( "canonical is false "){
	vector<string> kmers2 = generate_kmer(seq, k, 'N',false);
	for (string s : kmers2) {
		REQUIRE( k == s.size());
	}

	std::sort(kmers2.begin(),kmers2.end());
	REQUIRE (kmers2.size()==kmers1.size());
	REQUIRE ( kmers2 == kmers1);
}

	SECTION( "canonical is true"){
	vector<string> ckmers1;
	for (auto i: kmers1) {
		ckmers1.push_back(canonical_kmer(i));
	}
	std::sort(ckmers1.begin(),ckmers1.end());
	auto last = std::unique(ckmers1.begin(), ckmers1.end());
	ckmers1.erase(last, ckmers1.end());

	vector<string> kmers2 = generate_kmer(seq, k, 'N',true);
	for (string s : kmers2) {
		REQUIRE( k == s.size());
	}

	std::sort(kmers2.begin(),kmers2.end());

	REQUIRE (kmers2.size()==ckmers1.size());
	REQUIRE ( kmers2 == ckmers1);
}
}

TEST_CASE( "test with random kmer", "[kmer]" ) {
	string seq = "ATCGACGTNACNACTGGTAC";
	int k = 5;
	string kmer = random_generate_kmer(seq, k, false);
	REQUIRE(5 == kmer.size());

	kmer = random_generate_kmer(seq, k, true);
	REQUIRE(5 == kmer.size());

}

TEST_CASE( "kmp search", "[kmp]" ) {

	string word = "AGGGCAACAGAA";
	string text =
			"AGGGCAACAGAAGATGATAAAGGGTAACCGAACAGTTTCTTTGCCCTGGAGGGCAACAGAAGGCTCAGCTCGATCCGGTTAGGGCAACAGGATCCTCTAGGGCAATAGAAAGCCTCTTCCGGATCACCGAGGGCAACAGAA";

	SECTION ("bug fix section"){
	vector<int> found_indexes;

	word="A";
	text ="AGCTAGCT";
	kmpSearch(word,text, found_indexes);
	REQUIRE (found_indexes.size()==2);
	REQUIRE (found_indexes[0]==0);
	REQUIRE (found_indexes[1]==4);

}
	SECTION( "kmp search and error free pigeonhole kmp"){

	vector<int> found_indexes;
	kmpSearch("", "AGAT", found_indexes);
	REQUIRE (found_indexes.size()==0);
	found_indexes = pigeonhole_knuth_morris_pratt_search("","AGAT",0);
	REQUIRE (found_indexes.size()==0);

	kmpSearch("ACGT", "", found_indexes);
	REQUIRE (found_indexes.size()==0);
	found_indexes = pigeonhole_knuth_morris_pratt_search("AGAT","",0);
	REQUIRE (found_indexes.size()==0);

	kmpSearch("AGCT", "AGAT", found_indexes);
	REQUIRE (found_indexes.size()==0);
	found_indexes = pigeonhole_knuth_morris_pratt_search("AGCT","AGAT",0);
	REQUIRE (found_indexes.size()==0);

	kmpSearch("AGCT", "AGCT", found_indexes);
	REQUIRE (found_indexes.size()==1);
	REQUIRE (found_indexes[0]==0);
	found_indexes = pigeonhole_knuth_morris_pratt_search("AGCT","AGCT",0);
	REQUIRE (found_indexes.size()==1);
	REQUIRE (found_indexes[0]==0);

	kmpSearch("AGAG", "AGAGAG", found_indexes);
	REQUIRE (found_indexes.size()==2);
	REQUIRE (found_indexes[0]==0);
	REQUIRE (found_indexes[1]==2);
	found_indexes = pigeonhole_knuth_morris_pratt_search("AGAG","AGAGAG",0);
	REQUIRE (found_indexes.size()==2);
	REQUIRE (found_indexes[0]==0);
	REQUIRE (found_indexes[1]==2);

	kmpSearch("AT", "AAAAAAAAT", found_indexes);
	REQUIRE (found_indexes.size()==1);
	found_indexes = pigeonhole_knuth_morris_pratt_search("AT","AAAAAAAAT",0);
	REQUIRE (found_indexes.size()==1);

	kmpSearch("AGAG", "TAGAGAGAGCAGAG", found_indexes);
	REQUIRE (found_indexes.size()==4);
	found_indexes = pigeonhole_knuth_morris_pratt_search("AGAG","TAGAGAGAGCAGAG",0);
	REQUIRE (found_indexes.size()==4);

	kmpSearch(word,text,found_indexes);
	REQUIRE (found_indexes.size()==3);
	found_indexes = pigeonhole_knuth_morris_pratt_search(word,text,0);
	REQUIRE (found_indexes.size()==3);

}

	SECTION( "pigeonhole kmp with errors"){

	vector<int> found_indexes;

	found_indexes = pigeonhole_knuth_morris_pratt_search("","AGAT",1);
	REQUIRE (found_indexes.size()==0);

	found_indexes = pigeonhole_knuth_morris_pratt_search("AGAT","",1);
	REQUIRE (found_indexes.size()==0);

	found_indexes = pigeonhole_knuth_morris_pratt_search("AGCT","AGAT",1);
	REQUIRE (found_indexes.size()==1);
	REQUIRE (found_indexes[0]==0);

	found_indexes = pigeonhole_knuth_morris_pratt_search("AGCT","AGATCT",1);
	REQUIRE (found_indexes.size()==2);
	REQUIRE (found_indexes[0]==0);
	REQUIRE (found_indexes[1]==2);

	found_indexes = pigeonhole_knuth_morris_pratt_search("AGCT","AGATATCG",2);
	REQUIRE (found_indexes.size()==3);
	REQUIRE (found_indexes[0]==0);
	REQUIRE (found_indexes[1]==2);
	REQUIRE (found_indexes[2]==4);

	found_indexes = pigeonhole_knuth_morris_pratt_search(word,text,1);
	std::cout << "error allowed: " << 1 << std::endl;
	for (auto i : found_indexes) {
		std::cout << i <<'\t' << text.substr(i,word.length()) << ' ' << word << std::endl;
	}
	REQUIRE (found_indexes.size()==5);

	found_indexes = pigeonhole_knuth_morris_pratt_search(word,text,2);
	std::cout << "error allowed: " << 2 << std::endl;
	for (auto i : found_indexes) {
		std::cout << i <<'\t' << text.substr(i,word.length()) << ' ' << word << std::endl;
	}
	REQUIRE (found_indexes.size()==6);

}

}

TEST_CASE( "string overlap", "[overlap]" ) {

	SECTION("left to right overlap without error"){

	cout << "without error" << endl;
	string left, right;
	int pos;

	left ="AGCTAGCTA";
	right ="AGCTTTT";
	pos = left_right_overlap(left,right,4,0.0f);
	REQUIRE (pos==-1);

	left ="AGCTAGCT";
	right ="AGCTTTT";
	pos = left_right_overlap(left,right,4,0.0f);
	overlap_print(left,right,pos);
	REQUIRE (pos==4);

	left ="AGCTAGCTAGCT";
	right ="AGCTTTT";
	pos = left_right_overlap(left,right,4,0.0f);
	overlap_print(left,right,pos);
	REQUIRE (pos==8);

	left ="AGCTAGCTAGCT";
	right ="AGCTAGCTTTT";
	pos = left_right_overlap(left,right,4,0.0f);
	overlap_print(left,right,pos);
	REQUIRE (pos==4);

}

SECTION("left to right overlap allowing 1 error") {

	cout << "allows 1 error" << endl;
	string left, right;
	int pos;

	left ="AGCTAGCT";
	right ="AGCCTTT";
	pos = left_right_overlap(left,right,4,0.3f);
	overlap_print(left,right,pos);
	REQUIRE (pos==4);

	left ="AGCTAGCT";
	right ="AACTTTT";
	pos = left_right_overlap(left,right,4,0.3f);
	overlap_print(left,right,pos);
	REQUIRE (pos==4);

	left ="AGCTAGCTAACT";
	right ="AACTAACTTTT";
	pos = left_right_overlap(left,right,4,0.3f);
	overlap_print(left,right,pos);
	REQUIRE (pos==4);
}

SECTION("left to right overlap allowing 2 error") {

	cout << "allows 2 error" << endl;
	string left, right;
	int pos;

	left ="AGCTAGCTA";
	right ="AGCCTTT";
	pos = left_right_overlap(left,right,4,0.5f);
	overlap_print(left,right,pos);
	REQUIRE (pos==4);

	left ="AGCTAGCT";
	right ="CACTTTT";
	pos = left_right_overlap(left,right,4,0.5f);
	overlap_print(left,right,pos);
	REQUIRE (pos==4);

}

SECTION("test max_overlap") {

	cout << "test overlap" << endl;
	string left, right;
	int n;
	left ="AGCTAGCTA";
	right ="AGCCTTT";
	n = sequence_overlap(left,right,4,0.3f);
	cout << left <<"\t" << right << "\t" << n <<endl;
	REQUIRE ( n==5 );

	left ="AGCTAGCTAGCT";
	right ="AGCTAGCT";
	n = sequence_overlap(left,right,4,0.3f);
	cout << left <<"\t" << right << "\t" << n <<endl;
	REQUIRE ( n==8 );

	left ="AGCTAGCT";
	right ="AGCTAGCT";
	n = sequence_overlap(left,right,4,0.3f);
	cout << left <<"\t" << right << "\t" << n <<endl;
	REQUIRE ( n==8 );

	left ="ATCGATCG";
	right ="ATCGATCGATCG";
	n = sequence_overlap(left,right,4,0.3f);
	cout << left <<"\t" << right << "\t" << n <<endl;
	REQUIRE ( n==8 );

}

}

TEST_CASE( "combination", "[combination]" ) {

	SECTION("full combination"){
	std::vector<std::pair<int,int> ,std::allocator<std::pair<int,int> > > results;
	std::vector<int> arr;

	arr = {};
	results = generate_edges(arr, 30);
	REQUIRE (results.size() == 0);

	arr = {2};
	results = generate_edges(arr, 30);
	REQUIRE (results.size() == 0);

	arr = {2,1};
	results = generate_edges(arr, 30);
	REQUIRE (results.size() == 1);

	arr = {4,20,3,10};
	results = generate_edges(arr, 30);
	printf("full combination 1\n");
	for (auto u: results) {
		printf ("%d, %d\n", u.first,u.second);
	}
	REQUIRE (results.size() == 6);

}

SECTION("partial combination") {
	std::vector<std::pair<int,int> ,std::allocator<std::pair<int,int> > > results;
	std::vector<int> arr;

	arr = {};
	results = generate_edges(arr, 0);
	REQUIRE (results.size() == 0);

	arr = {2};
	results = generate_edges(arr, 1);
	REQUIRE (results.size() == 0);

	arr = {4,20,3,10,100,200,121,321};
	results = generate_edges(arr, 6);
	printf("partial combination 1\n");
	for (auto u: results) {
		printf ("%d, %d\n", u.first,u.second);
	}
	REQUIRE (results.size() <= arr.size()*3);

}

SECTION("base64 and long") {

	cout << "base64 and long" << endl;
	unsigned long n;
	std::string str;

	n=0;
	str=ulong2base64(n);
	REQUIRE (str=="A");
	REQUIRE(n==base64toulong(str));

	n=12358;
	str=ulong2base64(n);
	cout << n <<"\t" << str << endl;
	REQUIRE(n==base64toulong(str));

	n=3223212358;
	str=ulong2base64(n);
	cout << n <<"\t" << str << endl;
	REQUIRE(n==base64toulong(str));

	n=83223212358;
	str=ulong2base64(n);
	cout << n <<"\t" << str << endl;
	REQUIRE(n==base64toulong(str));
}

}
