#include "cache_structs.hpp"
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <set>
#include <fstream>
#include <exception>
#include <iostream>
using namespace std;

long long hex2dec(std::string hex)
{
    long long result = 0;
    for (int i=0; i<hex.length(); i++) {
        if (hex[i]>=48 && hex[i]<=57)
        {
            result += (hex[i]-48)*pow(16,hex.length()-i-1);
        } else if (hex[i]>=65 && hex[i]<=70) {
            result += (hex[i]-55)*pow(16,hex.length( )-i-1);
        } else if (hex[i]>=97 && hex[i]<=102) {
            result += (hex[i]-87)*pow(16,hex.length()-i-1);
        }
    }
    return result;
}

std::vector<std::pair<char, long long> > readTrace(std::ifstream& traceFile) {
    std::vector<std::pair<char, long long> > trace;
    std::string line;
    while (std::getline(traceFile, line)) {
        char* str_line;
        char inst_type;
        sscanf(line.c_str(), "%c %s", &inst_type, str_line);
        long long pc = hex2dec(str_line);
        trace.push_back(std::make_pair(inst_type, pc));
    }
    return trace;
}



int main(int argc, char* argv[]){
	
	if (argc != 7) {
        std::cout << "Usage: ./a <Block_size> <L1_size> <L1_assoc> <L2_size> <L2_assoc> <file_name>\n";
        return 1;
    }
    
    long long blk_size = std::stoi(argv[1]);
    long long l1_size = std::stoi(argv[2]);
    long long l1_ass = std::stoi(argv[3]);
    long long l2_size = std::stoi(argv[4]);
    long long l2_ass = std::stoi(argv[5]);
    
    long long l1_sets = l1_size / (blk_size * l1_ass);
    long long l2_sets = l2_size / (blk_size * l2_ass);
    
    struct l1_cache l1(l1_sets, blk_size, l1_ass);
    struct l2_cache l2(l2_sets, blk_size, l2_ass);
     
    std::ifstream traceFile(argv[6]);
    std::vector<std::pair<char, long long> > trace = readTrace(traceFile);

    // l1.checker(trace[1].second);
    long long l1_reads = 0;
    long long l1_read_misses = 0;
    long long l1_writes = 0;
    long long l1_write_misses = 0;
    long long l1_writebacks = 0;
    
    long long l2_reads = 0;
    long long l2_read_misses = 0;
    long long l2_writes = 0;
    long long l2_write_misses = 0;
    long long l2_writebacks = 0;
    
    long long total_access_time = 0;
    
    
    for (int i = 0; i < trace.size(); i++){
        
        
        
        char type = trace[i].first;
        long long pc = trace[i].second;
        
        if (type == 'r'){
            // cout << "r";
            
            l1_reads++;
            // total_access_time += 1;
            
            if (! l1.read(pc)){
                // cout << "why";
                l1_read_misses++;
                
                if (l1.check_full(pc)){
                    pair<bool, pair<long long, long long> > evict_from_l1 = l1.evict(pc);
                    // cout << endl << "checking";
                    // l1.lru_print(pc);
                    // cout << endl;
                    if (evict_from_l1.first == true){
                        // the dirty bit is 1
                        // need to send the write request at this point
                        l2_writes++;
                        // total_access_time += 20;
                        
                        // write into l2
                        long long evicted_set = evict_from_l1.second.first;
                        long long evicted_tag = evict_from_l1.second.second;
                        long long new_pc = (evicted_tag*l1_sets + evicted_set)*blk_size ;
                            
                        if (! l2.write(new_pc)){
                            l2_write_misses++;
                            
                            l2.insert(new_pc, 1);
                            // l2.easy_insert(evict_from_l1.second.first, evict_from_l1.second.second);
                        }else{
                            l2.read_update_lru(new_pc);
                        }
                        // if found then dirty bit set 1 automatically
                        
                    }
                }
                
                // total_access_time += 20;
                l2_reads++;
                
                // check if l2 misses                
                if (! l2.read(pc)){
                    l2_read_misses++;
                    
                    // make space in L2
                    if(l2.check_full(pc)){
                        pair<bool, pair<long long, long long> > evict_from_l2 = l2.evict(pc);
                        if (evict_from_l2.first == true){
                            // the dirty bit is 1
                            l2_writebacks++;
                            // write into memory which is automatic
                        }
                        // if dirty bit is 0 then do nothing
                        // else write in memory (only access time change)
                    }
                    // insert into vacated space of l2
                    l2.insert(pc, 0);
                    // insert into vacated space of l1
                    l1.insert(pc, 0);
                }else{
                    // if block is found in l2
                    
                    // insert into the vacated space of l1
                    l1.insert(pc, 0);
                    // dirty bit is with respect to the next level
                    
                    l2.read_update_lru(pc);
                }
                
                
                
            }else{
                l1.read_update_lru(pc);
            }
            
        }else{
            // cout << "w";
            
            l1_writes++;
            // total_access_time += 1;
            
            // check if l1 misses
            if (! l1.write(pc)){
                // cout << "here kya";
                
                l1_write_misses++;
                l2_reads++;
                
                // first make space in l1
                if (l1.check_full(pc)){
                    pair<bool, pair<long long, long long> > evict_from_l1 = l1.evict(pc);
                    
                    if (evict_from_l1.first == true){
                        // the dirty bit is 1
                        // need to send the write request at this point
                        l2_writes++;
                        // total_access_time += 20;
                        
                        // write into l2
                        long long evicted_set = evict_from_l1.second.first;
                        long long evicted_tag = evict_from_l1.second.second;
                        long long new_pc = (evicted_tag*l1_sets + evicted_set)*blk_size ;
                            
                        if (! l2.write(new_pc)){
                            l2_write_misses++;
                            
                            l2.insert(new_pc, 1);
                            
                            // l2.easy_insert(evict_from_l1.second.first, evict_from_l1.second.second);
                        }else{
                            l2.read_update_lru(new_pc);
                        }
                        // if found then dirty bit set 1 automatically
                        
                    }
                }
                
                // then proceed to checking in l2
                
                // check if l2 misses
                if (! l2.read(pc)){
                    l2_read_misses++;
                    
                    // make space in L2
                    if (l2.check_full(pc)){
                        
                        pair<bool, pair<long long, long long> > evict_from_l2 = l2.evict(pc);
                        if (evict_from_l2.first == true){
                            // the dirty bit is 1
                            l2_writebacks++;
                            // total_access_time += 200;
                            // write into memory which is automatic
                        }
                        // if dirty bit is 0 then do nothing
                        // else write in memory (only access time change)
                        
                    }
                    
                    // total_access_time += 200;
                    // insert into the vacated space of l2
                    l2.insert(pc, 0);
                    // insert into the vacated space of l1
                    l1.insert(pc, 1);
                    // dirty bit 1 because different value
                }else{
                    // if found in l2
                    
                    // insert into the vacated space of l1
                    // dirty bit will be 1 in this case
                    l1.insert(pc, 1);
                    
                    l2.read_update_lru(pc);
                }
            }else{
                l1.read_update_lru(pc);
            }
            // else the dirty bit is set to 1 by itself
            
        }
        
    }
    
    cout << "===== Simulation Results =====";
    

    cout << "\ni. number of L1 reads:\t\t\t\t" << dec << l1_reads;
    cout << "\nii. number of L1 read misses:\t\t\t" << dec << l1_read_misses;
    cout << "\niii. number of L1 writes:\t\t\t" << dec << l1_writes;
    cout << "\niv. number of L1 write misses:\t\t\t" << dec << l1_write_misses;
    cout << "\nv. L1 miss rate:\t\t\t\t" << fixed << setprecision(4) << ((float)l1_read_misses+(float)l1_write_misses)/ (l1_reads+l1_writes);
    cout << "\nvi. number of writebacks from L1 memory:\t" << dec << l2_writes;
    
    if (l2_size != 0)
    {
        cout << "\nvii. number of L2 reads:\t\t\t" << dec << l2_reads;
        cout << "\nviii. number of L2 read misses:\t\t\t" << dec << l2_read_misses;
        cout << "\nix. number of L2 writes:\t\t\t" << dec << l2_writes;
        cout << "\nx. number of L2 write misses:\t\t\t" << dec << l2_write_misses;
        cout << "\nxi. L2 miss rate:\t\t\t\t" << fixed << setprecision(4) << ((float)l2_read_misses+(float)l2_write_misses)/ (l2_reads+l2_writes);
        cout << "\nxii. number of writebacks from L2 memory:\t" << dec << l2_writebacks << "\n";
        // cout << "\nxiii. total access time:\t\t\t" << dec << (l1_reads + l1_writes)*1 + (l2_reads + l2_writes)*20 + (l2_read_misses + l2_write_misses + l2_writebacks)*200 << "ns\n";
    }
    
    // cout << "L1 Reads: " << l1_reads << endl;
    // cout << "L1 Read Misses: " << l1_read_misses << endl;
    // cout << "L1 Writes: " << l1_writes << endl;
    // cout << "L1 Write Misses: " << l1_write_misses << endl;
    // cout << "L2 Reads: " << l2_reads << endl;
    // cout << "L2 Read Misses: " << l2_read_misses << endl;
    // cout << "L2 Writes: " << l2_writes << endl;
    // cout << "L2 Write Misses: " << l2_write_misses << endl;
    // cout << "L2 Write Backs: " << l2_writebacks << endl;
    
    return 0;
}