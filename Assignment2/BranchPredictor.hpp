#ifndef __BRANCH_PREDICTOR_HPP__
#define __BRANCH_PREDICTOR_HPP__

#include <vector>
#include <bitset>
#include <stdlib.h>
#include <stdio.h>
#include <cassert> 
#include <unordered_map>
#include <string>

#include <functional>
#include <set>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>

struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector < std::bitset<2> > table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        int hash = pc % (1<<14); 
        std::bitset<2> counter = table[hash];
        if (counter.to_ulong() == 2 | counter.to_ulong() == 3) 
            { return true; }
        return false;
    }

    void update(uint32_t pc, bool taken) {
        int hash = pc % (1<<14); 
        std::bitset<2> counter = table[hash];

        if (taken == true) {
            int val;
            if (counter.to_ulong() == 3) {
                val = counter.to_ulong();
            }
            else {
                val = counter.to_ulong() + 1;
            }
            std::bitset<2> new_counter(val);
            table[hash] = new_counter;
        }

        else {
            int val;
            if (counter.to_ulong() == 0) {
                val = counter.to_ulong();
            }
            else {
                val = counter.to_ulong() - 1;
            }
            std::bitset<2> new_counter(val);
            table[hash] = new_counter;
        }
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector < std::bitset<2> > bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        int index = bhr.to_ulong();
        std::bitset<2> predset = bhrTable[index];
        int predcount = predset.to_ulong();

        if (predcount == 2 || predcount == 3) {
            return true; 
        }
        return false;
    }

    void update(uint32_t pc, bool taken) {
        int index = bhr.to_ulong();
        std::bitset<2> predset = bhrTable[index];
        int predcount = predset.to_ulong();

        if (taken == true) {
            int val;
            if (predcount == 3) val = predcount;
            else val = predcount + 1;
            std::bitset<2> new_counter(val);
            bhrTable[index] = new_counter;
            int LSB = bhr.test(0);
            int MSB = 1;
            std::bitset<2> answer;
            answer[0] = MSB;
            answer[1] = LSB;
            bhr = answer;  
        }
        else {
            int val;
            if (predcount == 0) val = predcount;
            else val = predcount - 1;
            std::bitset<2> new_counter(val);
            bhrTable[index] = new_counter;
            int LSB = bhr.test(0);
            int MSB = 0;
            std::bitset<2> answer;
            answer[0] = MSB;
            answer[1] = LSB;
            bhr = answer; 
        }
    }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector < std::bitset<2> > bhrTable;
    std::bitset<2> bhr;
    std::vector < std::bitset<2> > table;
    std::vector < std::bitset<2> > combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }

    bool predict(uint32_t pc) {
        combination.resize((1<<16));
        int hash = pc % (1<<14);
        int offset = bhr.to_ulong();
        int index = (hash * 4) + offset;
        std::bitset<2> predset = combination[index];
        int predcount = predset.to_ulong();
        if (predcount == 2 || predcount == 3 ) return true;
        return false;
    }

    void update(uint32_t pc, bool taken) {
        combination.resize((1<<16));
        int hash = pc % (1<<14);
        int offset = bhr.to_ulong();
        int index = (hash * 4) + offset;
        std::bitset<2> predset = combination[index];
        int predcount = predset.to_ulong();
        if (taken == 1) {
            int val;
            if (predcount == 3) val = predcount;
            else val = predcount + 1;
            std::bitset<2> new_counter(val);
            combination[index] = new_counter;
            int MSB = bhr.test(1);
            int LSB = 1;
            std::bitset<2> answer;
            answer[0] = MSB;
            answer[1] = LSB;
            bhr = answer;  
        }
        else {
            int val;
            if (predcount == 0) val = predcount;
            else val = predcount - 1;
            std::bitset<2> new_counter(val);
            combination[index] = new_counter;
            int MSB = bhr.test(1);
            int LSB = 0;
            std::bitset<2> answer;
            answer[0] = MSB;
            answer[1] = LSB;
            bhr = answer;  
        }
    }
};


#endif


