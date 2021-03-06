// Copyright (C) 2014 Hugo Gualdron <gualdron@usp.br>, Sao Paulo University
// Copyright (C) 2014 Jose Fernando Rodrigues Junior, Sao Paulo University
// Copyright (C) 2014 Duen Horng Chau, Georgia Tech University
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CORE_SPLITTERBUFFER_HPP_
#define CORE_SPLITTERBUFFER_HPP_

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../util/easylogging++.h"
#include "type.hpp"
#include "util.hpp"

namespace mflash{

template <class IdType, class EdgeSplitterManager_, class EdgeDataType = EmptyField>
class SplitterBuffer{

protected:
	/**
	 * Buffer size in elements
	 */
	int64 buffer_size;

	/**
	 * Edge size in bytes without include the size of the in-id and out-id.
	 */
	int64 edge_data_size;

	bool is_edge_data;

	/**
	 * Edge size in bytes.
	 */
	int64 edge_size;

	/**
	 *  Number of element in the buffer.
	 */
	int64 elements_buffer;

	int64 id_size;


	/**
	 *  Number of ids for partition. The edges are split using the ids.
	 *  Example:
	 *  Partition 0: 0 ... ids_by_partition -1
	 *  Partition 1: ids_by_partition+1 ... 2*ids_by_partition -1
	 *  ...
	 *  Partition partitions-1:  ids_by_partition*(partitions-1) ... (partitions)*ids_by_partition -1
	 *
	 */
	//int64 ids_by_partition;


	std::vector<int64> file_offsets;

	//int64 partitions;

	std::string graph;

	char *splitter_buffer;

	char *ptr_current_position;

	char *ptr_last_position;

	std::string file_prefix;

	bool cache_partitioning;

	EdgeSplitterManager_ * manager;

	void split();
	std::vector<int64>* block_counting(char* ptr_, char* last_ptr_, bool in_split);
	void block_sorting( char* ptr_, char* last_ptr_, bool in_split, std::vector<int64> & counters);

	//virtual IdType getPartitionId(IdType in_id, IdType out_id);

	//virtual IdType countEdge(IdType in_id, IdType out_id);

	//virtual std::string getPartitionFile(IdType id);

	void checkCounters();
	void checkCounters2(int64 partition_initial_positions[], int64 partition_offset_positions[]);

public:

	SplitterBuffer(EdgeSplitterManager_* manager, std::string graph, int64 edge_data_size, int64 buffer_size,  std::string file_prefix = "", bool cache_partitioning = true);

	void add(IdType in_id, IdType out_id, void* edge_data);

	void flush();


	virtual ~SplitterBuffer();
};

template <class IdType, class EdgeSplitterManager_, class EdgeDataType> inline
SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::SplitterBuffer(EdgeSplitterManager_* manager, std::string graph, int64 edge_data_size, int64 buffer_size, std::string file_prefix, bool cache_partitioning){
	this->manager = manager;
	this->graph = graph;
	this->edge_data_size = edge_data_size;
	this->edge_size =  2 * sizeof(IdType) + edge_data_size;
	this->is_edge_data = edge_data_size != 0;
	this->buffer_size = buffer_size;
	//this->ids_by_partition = manager->getIds_byP();
	this->file_prefix = file_prefix;
	this-> elements_buffer = buffer_size/ edge_size;
	this->id_size = sizeof(IdType);

	splitter_buffer = new char[edge_size * elements_buffer];
	ptr_current_position = splitter_buffer;
	ptr_last_position = splitter_buffer + edge_size * elements_buffer;

	this->file_offsets.resize(manager->getPartitions());// = new std::vector<int64>(partitions);
	this->cache_partitioning = cache_partitioning;

}


template <class IdType, class EdgeSplitterManager_, class EdgeDataType>
SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::~SplitterBuffer(){
	delete this->splitter_buffer;
}



/*template <class IdType, class EdgeSplitterManager_, class EdgeDataType>
void SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::checkCounters(){

	int64 counters[partitions];
	memset(counters, 0, sizeof(int64) * partitions);

	char* ptr = splitter_buffer->address();
	char* last_ptr = (splitter_buffer->address() + current_position * edge_size);

	while(ptr<last_ptr){
		counters[getPartitionId(*((IdType*)(ptr)), *((IdType*)(ptr + id_size)))]++;
		ptr+= edge_size;
	}
	LOG(INFO)<< "";

}*/
/*


template <class IdType, class EdgeSplitterManager_, class EdgeDataType>
void SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::checkCounters2(int64 partition_initial_positions[], int64 partition_offset_positions[]){

	int64 counters[partitions];
	memset(counters, 0, sizeof(int64) * partitions);

	char* ptr = splitter_buffer->address();
	char* last_ptr = (splitter_buffer->address() + current_position * edge_size);

	int64 current_partition = 0;
	int64 id;
	int64 count = 0;
	while(ptr<last_ptr){
		id = getPartitionId(*((IdType*)(ptr)), *((IdType*)(ptr + id_size)));
		if(id != current_partition){
			LOG(INFO) << "SORTED IN PARTITION "<< current_partition << ":"<< count << " OF "<<  (partition_offset_positions[current_partition]-  partition_initial_positions[current_partition])/8 ;
			current_partition ++;
			if(current_partition >= partitions){
				break;
			}
			ptr =  splitter_buffer->address() + partition_initial_positions[current_partition];
			count = 0;
		}else{
			count++;
			ptr+= edge_size;
		}
	}
	if(current_partition < partitions){
		LOG(INFO) << "SORTED IN PARTITION "<< current_partition << count << " OF "<<  (partition_offset_positions[current_partition]-  partition_initial_positions[current_partition])/8 ;
	}
	LOG(INFO)<< "";

}
*/


template <class IdType, class EdgeSplitterManager_, class EdgeDataType> inline
void SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::add(IdType in_id, IdType out_id, void* edge_data){
	if(ptr_current_position >= ptr_last_position){
		split();
	}
	manager->countEdge(in_id, out_id);
	memcpy(ptr_current_position, &in_id, id_size);
	memcpy(ptr_current_position + id_size, &out_id, id_size);
	if(is_edge_data ){
		memcpy(ptr_current_position+ (id_size<<1), edge_data, edge_data_size);
	}
	ptr_current_position += edge_size;
}



template <class IdType, class EdgeSplitterManager_, class EdgeDataType>
std::vector<int64>* SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::block_counting(char* ptr_, char* last_ptr_, bool in_split){
    IdType partition_id;

    IdType partitions = manager->getPartitions();
    std::vector<int64> *sub_counters = new std::vector<int64>(partitions);

    char* base_ptr = ptr_;
    int64 ptr = 0;
    int64 last_ptr = last_ptr_ - ptr_;

    int64 sub_counters_size = partitions;

    bool current_in_split = manager->isInSplit();
    manager->setInSplit(in_split);

    while(ptr < last_ptr){
        partition_id = manager->getPartitionId(*((IdType*)(ptr + base_ptr)), *((IdType*)(ptr + base_ptr+ id_size)));

        if(sub_counters_size < partition_id+1){
            sub_counters_size = partition_id+1;
            sub_counters->resize(sub_counters_size);
        }
        (*sub_counters)[partition_id]++;

        ptr += edge_size;
    }
    manager->setInSplit(current_in_split);

    return sub_counters;

}

template <class IdType, class EdgeSplitterManager_, class EdgeDataType>
void SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::block_sorting( char* ptr_, char* last_ptr_, bool in_split, std::vector<int64> & counters){
    IdType partition_id;
    char tmp_edge[edge_size];
    IdType partitions = manager->getPartitions();

    char* base_ptr = ptr_;
    int64 ptr = 0;
    int64 last_ptr = last_ptr_ - ptr_;

    int64 partition_initial_positions[partitions+1];
    int64 partition_offset_positions[partitions+1];
    partition_initial_positions[0] = 0;
    partition_offset_positions[0] = 0;

    for(int32 i = 1 ; i <= partitions; i++){
        partition_initial_positions[i] = partition_initial_positions[i-1] + edge_size * counters[i-1];
        partition_offset_positions[i] = partition_initial_positions[i];
    }
    partition_offset_positions[partitions] = 0;

    //setting last pointer to the first position of the last partion because it will be sorted
    last_ptr =  partition_initial_positions[partitions-1];

    bool current_in_split = manager->isInSplit();
    manager->setInSplit(in_split);

    int64 partition_ptr;
    while(ptr < last_ptr){
        partition_id = manager->getPartitionId(*((IdType*)(ptr + base_ptr)), *((IdType*)(ptr + base_ptr+ id_size)));

        partition_ptr = partition_offset_positions[partition_id];

        if(ptr != partition_ptr){
            memcpy(tmp_edge, base_ptr+ptr, edge_size);
            memcpy(base_ptr+ptr, base_ptr+partition_ptr ,edge_size);
            memcpy(base_ptr+partition_ptr, tmp_edge ,edge_size);

        }else{
            ptr += edge_size;
            if(ptr == partition_initial_positions[partition_id+1]){
                //moving the pointer to the next partition with edges
                while (partition_initial_positions[partition_id+2] - partition_initial_positions[partition_id+1] == 0)
                    partition_id++;
                //condition required when the next partition has sorted
                ptr = partition_offset_positions[partition_id+1] = max(partition_offset_positions[partition_id+1] - edge_size, partition_initial_positions[partition_id+1]);
            }

        }
        partition_offset_positions[partition_id]+= edge_size;
    }

    manager->setInSplit(current_in_split);

}



template <class IdType, class EdgeSplitterManager_, class EdgeDataType>
void SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::split(){


	//LOG(INFO) << "Splitting buffer";
	//splitting value
	char* base_ptr = splitter_buffer;
	int64 ptr = 0; //splitter_buffer->address();
	int64 last_ptr = 0; //ptr + current_position * edge_size;
	IdType partitions = manager->getPartitions();

	//int64 partition_initial_positions[partitions+1];
	std::vector<int64> & partition_counters = manager->getPartitionCounters();
	for(int32 i = 0 ; i < partitions; i++){
	    last_ptr += partition_counters[i];
	}
	last_ptr *= edge_size;

	//setting last pointer to the first position of the last partion because it will be sorted
	//last_ptr =  partition_initial_positions[partitions-1];

	block_sorting(base_ptr + ptr, base_ptr + last_ptr, manager->isInSplit(), partition_counters);


	file_offsets.resize(partitions);

	char* offset = base_ptr;
	std::string partition_file = "";
	//writing partitions to disk
	int32 file_id = -1;
	for(int32 i = 0 ; i < partitions; i++){
	    std::string tmp_partition_file = manager->getPartitionFile(i);
	    if(partition_file != tmp_partition_file){
	        partition_file = tmp_partition_file;
	        file_id = i;
	    }
		if(partition_counters[i] == 0){
			continue;
		}
		int64 partition_size = sizeof(char) * partition_counters[i] * edge_size;

		//sub_sorting
		if( cache_partitioning){
            std::vector<int64> *b_counters = block_counting(offset, offset+partition_size, !manager->isInSplit());
            block_sorting(offset + ptr, offset+partition_size, !manager->isInSplit(), *b_counters);
            delete b_counters;
		}


		FILE * pFile;
		std::string file = get_file(graph, file_prefix, partition_file);
		pFile = fopen (file.c_str(), file_offsets[file_id] == 0? "wb": "ab");
		//LOG(DEBUG) << "Storing edges in partition "<< i << " : "<< file;
		fseek(pFile, file_offsets[file_id], SEEK_SET);
		fwrite (offset,sizeof(char), partition_size, pFile);
		fclose (pFile);

		offset += partition_size;

		file_offsets[file_id] += partition_size;
		//resetting counter for the next split
		partition_counters[i] = 0;
	}
	ptr_current_position = splitter_buffer;

}


template <class IdType, class EdgeSplitterManager_, class EdgeDataType>
void SplitterBuffer<IdType, EdgeSplitterManager_, EdgeDataType>::flush(){
	split();
}



}
#endif
