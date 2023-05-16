#ifndef __CACHE_STRUCTS_HPP__
#define __CACHE_STRUCTS_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <set>
#include <fstream>
#include <exception>
#include <iostream>
using namespace std;

struct Cache {
};

struct l1_cache : public Cache {
    vector < vector< pair<long long, bool> > > table;
    long long blk_size;
    long long num_sets;
    long long ass;
    vector < vector <long long> > lru;
    
    l1_cache(long long sets, long long size, long long assoc) : table(sets), lru(sets) {
    	blk_size = size;
    	num_sets = sets;
    	ass = assoc;
    }
    
    long long get_set(long long pc){
    	long long set = (pc / blk_size) % num_sets;
    	return set;
    }
    
    long long get_tag(long long pc){
    	long long tag = pc / (blk_size * num_sets);
    	return tag;
    }
    
    bool check_full(long long pc){
    	long long tag = get_tag(pc);
    	long long set_num = get_set(pc);
    	
    	if (lru[set_num].size() < ass) return false;
    	return true;
    }
    
    long long get_least(long long set_num){
    	long long index = 0;
    	
    	for (int i = 0; i < lru[set_num].size(); i++){
    		if (lru[set_num][i] > lru[set_num][index]) index = i;
    	}
    	return index;
    }
    
    void read_update_lru(long long pc){
    	long long tag = get_tag(pc);
    	long long set_num = get_set(pc);
    	
    	long long index = 0;
    	
    	for (int i  = 0; i < table[set_num].size(); i++){
    		// if read is a hit
    		if (table[set_num][i].first == tag) index = i;
    		//if read is a miss
    	}
    	
    	for (int i = 0; i < lru[set_num].size(); i++){
    		// if (lru[set_num])
    		if (lru[set_num][i] < lru[set_num][index]) lru[set_num][i]++;
    	}
    	
    	lru[set_num][index] = 1;
    	
    }
    
    void update_lru(long long set, long long index){
    	for (int i = 0; i < lru[set].size(); i++){
    		lru[set][i]++;
    	}
    	
    	lru[set][index] = 1;
    }
    
    pair<bool, pair<long long, long long> > evict(long long pc){
    	long long set = get_set(pc);
    	
    	pair<bool, pair<long long, long long> > x;
    	
    	long long index = get_least(set);
    	
		x.first = table[set][index].second;
		x.second.first = set;
		x.second.second = table[set][index].first;
    	return x;
    }
    
    void insert(long long pc, bool dirty_bit){
    	long long tag = get_tag(pc);
    	long long set_num = get_set(pc);
    	
    	pair <long long, bool> x;
		x.first = tag;
		x.second = dirty_bit;
    	
    	if (table[set_num].size() < ass){
    		// if size is less than associativity
    		// insert the new pair into the table
    		for (int i = 0; i < table[set_num].size(); i++){
    			lru[set_num][i]++;
    		}
    		table[set_num].push_back(x);
    		lru[set_num].push_back(1);
    		
    		return;
    	}
    	
		long long least = get_least(set_num);
		table[set_num][least] = x;
		update_lru(set_num, least);
    }
    
    bool read(long long pc){
    	long long tag = get_tag(pc);
    	long long set = get_set(pc);
        
        for (int i = 0; i < table[set].size(); i++){
            if (table[set][i].first == tag) return true;
        }
    	
    	// for (pair<long long, bool> it : table[set]){
    	// 	// if read is a hit
    	// 	if (it.first == tag) return true;
    	// 	//if read is a miss
    	// }
    	return false;
    }
    
    bool write(long long pc){
    	long long tag = get_tag(pc);
    	long long set = get_set(pc);
    	
    	for (int i = 0; i < table[set].size(); i++){
    		if (table[set][i].first == tag){
    			table[set][i].second = true;
    			return true;
    		}
    	}
    	
    	return false;
    }
    
    void checker(long long pc){
    	cout << get_tag(pc) << endl;
    	cout << get_set(pc);
    }
    
    void cache_printer(){
    	for  (int i = 0; i < table.size(); i++){
    		cout << "SET " << i << ":";
    		for (int j = 0; j < table[i].size(); j++){
    			cout << table[i][j].first << " " << table[i][j].second << "|";
    		} 
    		cout << endl;
    	}
    }
    
    void lru_printer(){
    	for  (int i = 0; i < lru.size(); i++){
    		cout << "SET " << i << ":";
    		for (int j = 0; j < lru[i].size(); j++){
    			cout << lru[i][j] << "|";
    		} 
    		cout << endl;
    	}
    }
    
    
};

struct l2_cache : public Cache {
    vector < vector< pair<long long, bool> > > table;
    long long blk_size;
    long long num_sets;
    long long ass;
    vector < vector <long long> > lru;
    
    l2_cache(long long sets, long long size, long long assoc) : table(sets), lru(sets) {
    	blk_size = size;
    	num_sets = sets;
    	ass = assoc;
    }
        
    long long get_set(long long pc){
    	long long set_num = (pc / blk_size) % num_sets;
    	return set_num;
    }
    
    long long get_tag(long long pc){
    	long long tag = pc / (blk_size * num_sets);
    	return tag;
    }
    
    long long get_least(long long set_num){
    	long long index = 0;
    	
    	for (int i = 0; i < lru[set_num].size(); i++){
    		if (lru[set_num][i] > lru[set_num][index]) index = i;
    	}
    	return index;
    }
    
    bool check_full(long long pc){
    	long long tag = get_tag(pc);
    	long long set_num = get_set(pc);
    	
    	if (lru[set_num].size() < ass) return false;
    	return true;
    }
    
    
    void read_update_lru(long long pc){
    	long long tag = get_tag(pc);
    	long long set_num = get_set(pc);
    	
    	long long index = 0;
    	
    	for (int i  = 0; i < table[set_num].size(); i++){
    		// if read is a hit
    		if (table[set_num][i].first == tag) index = i;
    		//if read is a miss
    	}
    	
    	for (int i = 0; i < lru[set_num].size(); i++){
    		// if (lru[set_num])
    		if (lru[set_num][i] < lru[set_num][index]) lru[set_num][i]++;
    	}
    	
    	lru[set_num][index] = 1;
    	
    }
    
    void write_update_lru(long long set_num, long long tag){
    	
    	long long index = 0;
    	
    	for (int i  = 0; i < table[set_num].size(); i++){
    		// if read is a hit
    		if (table[set_num][i].first == tag) index = i;
    		//if read is a miss
    	}
    	
    	for (int i = 0; i < lru[set_num].size(); i++){
    		// if (lru[set_num])
    		if (lru[set_num][i] < lru[set_num][index]) lru[set_num][i]++;
    	}
    	
    	lru[set_num][index] = 1;
    	
    }
    
    void update_lru(long long set_num, long long index){
    	for (int i = 0; i < lru[set_num].size(); i++){
    		lru[set_num][i]++;
    	}
    	
    	lru[set_num][index] = 1;
    }
    
    pair<bool, pair<long long, long long> > evict(long long pc){
    	long long set_num = get_set(pc);
    	pair<bool, pair<long long, long long> > x;
    	long long index = get_least(set_num);
		x.first = table[set_num][index].second;
		x.second.first = set_num;
		x.second.second = table[set_num][index].first;
    	return x;
    }
    
    void insert(long long pc, bool dirty_bit){
    	long long tag = get_tag(pc);
    	long long set_num = get_set(pc);
    	
    	pair <long long, bool> x;
		x.first = tag;
		x.second = dirty_bit;
    	
    	if (table[set_num].size() < ass){
    		// if size is less than associativity
    		// insert the new pair into the table
    		for (int i = 0; i < table[set_num].size(); i++){
    			lru[set_num][i]++;
    		}
    		table[set_num].push_back(x);
    		lru[set_num].push_back(1);
    		
    		return;
    	}
    	
		long long least = get_least(set_num);
		table[set_num][least] = x;
		update_lru(set_num, least);
    }
    
    void easy_insert(long long set, long long tag){
    	pair <long long, bool> x;
		x.first = tag;
		x.second = true;
		
    	if (table[set].size() < ass){
    		for (int i = 0; i < table[set].size(); i++){
    			lru[set][i]++;
    		}
    		table[set].push_back(x);
    		lru[set].push_back(1);
    		return;
    	}
    	
    	// for (pair<long long, bool> it : table[set]){
		long long least = get_least(set);
		table[set][least] = x;
		update_lru(set, least);
    	// }
    }
    
    bool read(long long pc){
    	long long tag = get_tag(pc);
    	long long set = get_set(pc);
    	
    	for (int i = 0; i < table[set].size(); i++){
            if (table[set][i].first == tag) return true;
        }
    	return false;
    }
    
    bool write(long long pc){
    	long long tag = get_tag(pc);
    	long long set = get_set(pc);
    	
    	for (int i = 0; i < table[set].size(); i++){
    		if (table[set][i].first == tag){
    			table[set][i].second = true;
    			return true;
    		}
    	}
    	
    	return false;
    }
    
    void checker(long long pc){
    	cout << get_tag(pc) << endl;
    	cout << get_set(pc);
    }
    
    void cache_printer(){
    	for  (int i = 0; i < table.size(); i++){
    		cout << "SET " << i << ":";
    		for (int j = 0; j < table[i].size(); j++){
    			cout << table[i][j].first << " " << table[i][j].second << "|";
    		} 
    		cout << endl;
    	}
    }
    
    void lru_printer(){
    	for  (int i = 0; i < lru.size(); i++){
    		cout << "SET " << i << ":";
    		for (int j = 0; j < lru[i].size(); j++){
    			cout << lru[i][j] << "|";
    		} 
    		cout << endl;
    	}
    }
    
};

#endif