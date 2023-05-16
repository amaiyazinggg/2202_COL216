#include "BranchPredictor.hpp"
#include <vector>
#include <bitset>
#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <iostream>
#include <sstream>


uint32_t hex2dec(std::string hex)
{
    uint32_t result = 0;
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

int main(int argc, char *argv[]) {
	struct SaturatingBHRBranchPredictor bpstruct(3, 1<<16);

	int taken;
	uint32_t pc;
	std::string pc_str;
	int actual;
	std::stringstream stream;
	int count = 0;
	int correct_count = 0;

	std::ifstream file(argv[1]);

	if (file.is_open()){
		std::string line;
		while (getline(file, line)){
			actual = line[9] - 48;
			pc_str =  line.substr(0, line.find(' '));
			pc = hex2dec(pc_str);
			
			taken = bpstruct.predict(pc);
			// std::cout << "pc is " << pc << " " << actual << " " << taken << std::endl;
			if (taken == actual) correct_count++;
			bpstruct.update(pc, actual);
			count++;
		}
		float accuracy = float (correct_count) / float (count);
		std::cout << "accuracy is " << correct_count << " " << count << std::endl;
		printf("%6.4lf", accuracy*100);
		std::cout << std::endl;
		file.close();
	}
	else
	{
		std::cerr << "File could not be opened. Terminating...\n";
		return 0;
	}

	return 0;
}
