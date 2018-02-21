// Copyright (C) 2014 Hugo Gualdron <gualdron@usp.br>, Sao Paulo University
// Copyright (C) 2014 Jose Fernando Rodrigues Junior, Sao Paulo University
// Copyright (C) 2014 Duen Horng Chau, Georgia Tech University
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "../src/mflash_basic_includes.hpp"
#include "../src/algorithm/pagerank.hpp"

#include <set>

using namespace std;
using namespace mflash;

const size_t MAX_SIZE = 20;

template <class V, class IdType>
class PrintVector: public ZeroOperator<V,IdType>{
    public:
        std::set<std::pair<float, int64>, std::greater<std::pair<float, int64>>> maxes;
        inline void apply(Element<V, IdType> &out) {
            auto val = *out.value;
            if (maxes.size() < MAX_SIZE) {
                maxes.insert({val, out.id});
                return;
            }
            auto last_it = std::prev(maxes.end());
            if (last_it->first < val) {
                maxes.erase(last_it);
                maxes.insert({val, out.id});
            }
        }
};

int main(int argc, char ** argv){
    mflash_init(argc, argv);

    /* Parameters */
    std::string filename    = get_option_string("file"); // Base filename
    int niters              = get_option_int("niters", 4);

    std::string pg_ranks 	= get_parent_directory(filename) + "pg";

    Matrix<EmptyField, int64> matrix (filename);
    PrimitiveVector<float, int64> pvector(pg_ranks);
    PageRank::run(matrix, pvector, niters);

	LOG(INFO) << "computing the best pagerank";
    auto p = PrintVector<float, int64>{};
    pvector.operate(p);

	LOG(INFO) << "== best pagerank";
    for (const auto& v: p.maxes) {
	    LOG(INFO) << v.second << " " << v.first;
    }
    return 0;
}
