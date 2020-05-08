#include <string>
#include <iostream>
#include <algorithm>
#include <cstdint>

#include <boost/lexical_cast.hpp>

#include "hash_delimeter.hpp"
#include "tokenizer.hpp"

using namespace imgdupl;

int
hamming_distance(uint64_t hash1, uint64_t hash2)
{
    uint64_t x = hash1 ^ hash2;

    return __builtin_popcountll(x);
}

int
dist(const std::string& h1, const std::string& h2)
{
    Tokens mh1, mh2;

    tokenize(h1, mh1, HASH_PRINT_DELIMETER);
    tokenize(h2, mh2, HASH_PRINT_DELIMETER);

    // FIXME: sanity check that mh1.size() == mh2.size() needed

    size_t hash_size = mh1.size();
    int dist = 0;

    for (size_t i = 0; i < hash_size; i++) {
        dist += hamming_distance(boost::lexical_cast<uint64_t>(mh1[i]), boost::lexical_cast<uint64_t>(mh2[i]));
    }

    return dist;
}

int
main(int argc, char** argv)
{
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << "<hash1> <hash2>" << std::endl;
        exit(0);
    }

    try {
        std::string hash_string1 = argv[1];
        std::string hash_string2 = argv[2];

        std::cout << "Hamming distance: " << dist(hash_string1, hash_string2) << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
