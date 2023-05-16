#ifndef __5STAGE_BYPASS_HPP__
#define __5STAGE_BYPASS_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <set>
#include <utility>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>

struct stage
	{
		std::string stage_name;
		std::vector<std::string> instruct;
		int nop = 0;
		int occupied = 0;
	};

struct MIPS_Architecture
{
	int registers[32] = {0}; // an array of 32 values all initialised to 0
	int PCcurr = 0; // Current PC Counter set to 0
	int PCnext ; // Next PC counter unitiated
	int storage[32] = {0}; // temporary register values corresponding to each register for bypassing
	
	std::unordered_map<std::string, std::function < int(MIPS_Architecture &, std::string, std::string, std::string)> > instructions;
	//instructions is an unordered map from string -> function
	
	std::unordered_map<std::string, int> registerMap, registerValueChange, address;
	//address and registerMap are an unordered maps from string -> int
	
	static const int MAX = (1 << 20);
	// MAX is a static constant equal to 2^20
	
	int data[MAX >> 2] = {0};
	// data is an array of size 2^18?
	
	std::vector<std::vector < std::string> > commands;
	// commands is a vector of vectors of strings (list of lists of strings)
	
	std::vector<int> commandCount;
	//commandCount is a list of int
	
	int start_from_here = 0;
	
	std::set<int> changed_addresses;
	std::vector<int> mem_writes;
	std::vector<int> values;
	
	int ID_visited = 0;
	int flag = 0;
	int force_stall = 0;
	int stall = 0;
	int choice = 0; // choose values from registers or stalls
	
	
	struct stage *N = new stage;
	struct stage *IF = new stage;
	struct stage *ID = new stage;
	struct stage *EX = new stage;
	struct stage *MEM = new stage;
	struct stage *WB = new stage;
	struct stage *X = new stage;
	
	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};
	//exit_code is data of type enumerate
	//starting with 0 each of the names have values 1,2,3...

	// constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};
		//all instructions have been defined
		
		for (int i = 0; i < 32; ++i){
			registerMap["$" + std::to_string(i)] = i;
			// registerAccess["$" + std::to_string(i)] = 0;
			registerValueChange["$" + std::to_string(i)] = 0;
		}
		//$1 mapped to 1, $2 mapped to 2 and so on
		
		registerMap["$zero"] = 0; //$zero mapped to 0
		registerMap["$at"] = 1; 
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		
		// registerAccess["$zero"] = 0;
		// registerAccess["$at"] = 0; 
		// registerAccess["$v0"] = 0;
		// registerAccess["$v1"] = 0;
		
		registerValueChange["$zero"] = 0;
		registerValueChange["$at"] = 0; 
		registerValueChange["$v0"] = 0;
		registerValueChange["$v1"] = 0;
		
		for (int i = 0; i < 4; ++i){
			registerMap["$a" + std::to_string(i)] = i + 4; //$a0 is 4 .. $a3 is 7
			// registerAccess["$a" + std::to_string(i)] = 0;
			registerValueChange["$a" + std::to_string(i)] = 0;
		}
		
		for (int i = 0; i < 8; ++i){
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
			// registerAccess["$t" + std::to_string(i)] = 0, registerAccess["$s" + std::to_string(i)] = 0;
			registerValueChange["$t" + std::to_string(i)] = 0, registerValueChange["$s" + std::to_string(i)] = 0;
		}
		//$t0 is 8 ... $t3 is 15
		//$s0 is 16 ... $s3 is 23
		
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;
		//rest of the register mapping is done here
		
		// registerAccess["$t8"] = 0;
		// registerAccess["$t9"] = 0;
		// registerAccess["$k0"] = 0;
		// registerAccess["$k1"] = 0;
		// registerAccess["$gp"] = 0;
		// registerAccess["$sp"] = 0;
		// registerAccess["$s8"] = 0;
		// registerAccess["$ra"] = 0;
		
		registerValueChange["$t8"] = 0;
		registerValueChange["$t9"] = 0;
		registerValueChange["$k0"] = 0;
		registerValueChange["$k1"] = 0;
		registerValueChange["$gp"] = 0;
		registerValueChange["$sp"] = 0;
		registerValueChange["$s8"] = 0;
		registerValueChange["$ra"] = 0;
		
		N -> stage_name = "N";
		X -> stage_name = "X";
		IF -> stage_name = "IF";
		ID -> stage_name = "ID";
		EX -> stage_name = "EX";
		MEM -> stage_name = "MEM";
		WB -> stage_name = "WB";
		
		

		constructCommands(file); //parses and stores every command in the commands vector
		commandCount.assign(commands.size(), 0); //stores how many times each command is run
	}

	// perform add operation
	int add(std::string r1, std::string r2, std::string r3)
	{
		// std::cout << "came here " << choice;
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });		
	} //[] is the  lambda operator in cpp.
	// [&] captures all the variables by reference

	// perform subtraction operation
	int sub(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a - b; });
	}

	// perform multiplication operation
	int mul(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a * b; });
	}

	// perform the binary operation
	int op(std::string r1, std::string r2, std::string r3, std::function<int(int, int)> operation)
	{
		// std::cout << "check check " << choice << std::endl;
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0) return 1;
			// std::cout << "hellu " << choice << std::endl;

		if (choice == 0) {// WB: write to registers 
			// std::cout << "why here " << choice << std::endl;
			registers[registerMap[r1]] = operation(registers[registerMap[r2]], registers[registerMap[r3]]); 
		}else{
			// std::cout << "hello" << std::endl;
		    storage[registerMap[r1]] = operation(storage[registerMap[r2]], storage[registerMap[r3]]);
		}

		return 0;
	}

	// perform the beq operation
	int beq(std::string r1, std::string r2, std::string label) {
		// std::cout << "yahan aaya" << choice << std::endl;
	
		return bOP(r1, r2, label, [](int a, int b)
				   { return a == b; });
	}

	// perform the bne operation
	int bne(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a != b; });
	}

	// implements beq and bne by taking the comparator
	int bOP(std::string r1, std::string r2, std::string label, std::function<bool(int, int)> comp)
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		if (!checkRegisters({r1, r2}))
			return 1;
		// PCnext = comp(registers[registerMap[r1]], registers[registerMap[r2]]) ? address[label] : PCcurr + 1;
		PCcurr = comp(storage[registerMap[r1]], storage[registerMap[r2]]) ? address[label] : PCcurr ;
		return 0;
	}

	// implements slt operation
	int slt(std::string r1, std::string r2, std::string r3)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		if (choice == 0)
		registers[registerMap[r1]] = registers[registerMap[r2]] < registers[registerMap[r3]];
		else storage[registerMap[r1]] = storage[registerMap[r2]] < storage[registerMap[r3]];
		// PCnext = PCcurr + 1;
		return 0;
	}

	// perform the jump operation
	int j(std::string label, std::string unused1 = "", std::string unused2 = "")
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		// PCnext = address[label];
		PCcurr = address[label];

		return 0;
	}

	// perform load word operation
	int lw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r) || registerMap[r] == 0)
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		if (choice == 0)
		registers[registerMap[r]] = data[address];
		else 
		storage[registerMap[r]] = data[address];
		// PCnext = PCcurr + 1;
		return 0;
	}

	// perform store word operation
	int sw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r))
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		data[address] = registers[registerMap[r]];
		// PCnext = PCcurr + 1;
		return 0;
	}

	int locateAddress(std::string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = registers[registerMap[reg]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
					return -3;
				return address / 4;
			}
			catch (std::exception &e)
			{
				return -4;
			}
		}
		try
		{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				return -3;
			return address / 4;
		}
		catch (std::exception &e)
		{
			return -4;
		}
	}

	// perform add immediate operation
	int addi(std::string r1, std::string r2, std::string num)
	{
		if (!checkRegisters({r1, r2}) || registerMap[r1] == 0)
			return 1;
		try
		{
			if (choice == 0) registers[registerMap[r1]] = registers[registerMap[r2]] + stoi(num);
			else storage[registerMap[r1]] = storage[registerMap[r2]] + stoi(num);
			
			// PCnext = PCcurr + 1;
			return 0;
		}
		catch (std::exception &e)
		{
			return 4;
		}
	}

	// checks if label is valid
	inline bool checkLabel(std::string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			std::cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				std::cerr << s << ' ';
			std::cerr << '\n';
		}
		std::cout << "\nFollowing are the non-zero data values:\n";
		for (int i = 0; i < MAX / 4; ++i)
			if (data[i] != 0)
				std::cout << 4 * i << '-' << 4 * i + 3 << std::hex << ": " << data[i] << '\n'
						  << std::dec;
		std::cout << "\nTotal number of cycles: " << cycleCount << '\n';
		std::cout << "Count of instructions executed:\n";
		for (int i = 0; i < (int)commands.size(); ++i)
		{
			std::cout << commandCount[i] << " times:\t";
			for (auto &s : commands[i])
				std::cout << s << ' ';
			std::cout << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}

	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	} //at the end we have all instructions in the commands vector
	
	
	struct stage* give_next_stage(struct stage* curr){
		if (curr -> stage_name == "N"){
			IF -> occupied = 1;
			return IF;
		}else if (curr -> stage_name == "IF"){
			IF -> occupied = 0;
			ID -> occupied = 1;
			return ID;
		}else if (curr -> stage_name == "ID"){
			ID -> occupied = 0;
			EX -> occupied = 1;
			return EX;
		}else if (curr -> stage_name == "EX"){
			EX -> occupied = 0;
			MEM -> occupied = 1;
			return MEM;
		}else if (curr -> stage_name == "MEM"){
			MEM -> occupied = 0;
			WB -> occupied = 1;
			return WB;
		}else if (curr -> stage_name == "WB"){
			WB -> occupied = 0;
			return X;
		}else{
			return X;
		}
	}
	
	// CHECKED : gives register value from the sw/lw type 20($1) etc.
	std::string get_register(std::string location){
		int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
		std::string reg = location.substr(lparen + 1);
		reg.pop_back();
		return reg;
	}
	
	void start_IF(){
		// std::cout << "Stall is " << stall << " ";
		if (stall != 0){
			return;
		}
		if (IF -> occupied == 0){
			IF -> instruct = commands[PCcurr];
			// std::cout << "PC taken in IF is " << PCcurr << " ";
			IF -> occupied = 1;
			PCcurr++;
			 // so that same instruction runs again (case of curr)
		}
	}
	
	//whichever is occupied we run it
	void run_IF(){
		// std::cout << " IF ran " << std::endl;
		
		if (IF -> nop == 1){
			if (ID -> occupied == 0){
				// std::cout << "aao balam" <<std::endl;
				// if ((IF -> instruct)[0] == "bne" || (IF -> instruct)[0] == "beq" || ((IF -> instruct)[0] == "j")) stall++;
				ID -> occupied = 1;
				IF -> occupied = 0;
				IF -> nop = 0;
				ID -> nop = 1;
				ID -> instruct = IF -> instruct;
				return;
			}
		}
		
		if (ID -> occupied == 0){
			if ((IF -> instruct)[0] == "bne" || (IF -> instruct)[0] == "beq" || ((IF -> instruct)[0] == "j")) stall++;
			ID -> occupied = 1;
			IF -> occupied = 0;
			ID -> instruct = IF -> instruct;	
			return;
		}
	}
	
	void run_ID(){
		//bneq etc set nop here only
		// std::cout << " ID ran " << std::endl; 
		if (ID -> nop == 1){
			if (EX -> occupied == 0){
				// std::cout << "teri kasam" << std::endl;
				ID -> occupied = 0;
				EX -> occupied = 1;
				EX -> instruct = ID -> instruct;
				EX -> nop = 1;
				ID -> nop = 0;
				return;
			}
		}
		
		if (EX -> occupied == 0){
			if ((ID -> instruct)[0] == "addi"){
				std::string reg1 = (ID -> instruct)[1];
				std::string reg2 = (ID -> instruct)[2];
				// std::cout << "debug crow" << std::endl;
				if (registerValueChange[reg2] == 0){
					registerValueChange[reg1] = 1;
					ID -> occupied = 0;
					EX -> occupied = 1;
					EX -> instruct = ID -> instruct;
					// std::cout << "karooooo" << std::endl;
					// if (flag == 1){
					// 	flag = 0;
					// 	run_EX();
					// }
					return;
				} else flag = 1;
			}else if (((ID -> instruct)[0] == "add") || ((ID -> instruct)[0] == "sub") || ((ID -> instruct)[0] == "mul") || ((ID -> instruct)[0] == "slt") ){
				std::string reg1 = (ID -> instruct)[1];
				std::string reg2 = (ID -> instruct)[2];
				std::string reg3 = (ID -> instruct)[3];
				// std::cout << " here " ;
				
				if ((registerValueChange[reg2] == 0) && (registerValueChange[reg3] == 0)){
					// std::cout << " should be here " ;
					registerValueChange[reg1] = 1;
					ID -> occupied = 0;
					EX -> occupied = 1;
					EX -> instruct = ID -> instruct;
					// if (flag == 1){
					// 	flag = 0;
					// 	run_EX();
					// }
					return;
				}else flag = 1;
			}else if ((ID -> instruct)[0] == "lw"){
				std::string reg2 = get_register((ID -> instruct)[2]);
				std::string reg1 = (ID -> instruct)[1];
				int new_address = locateAddress((ID->instruct)[2]);
				
				if (registerValueChange[reg2] == 0){
					// std::cout << "atleast yaha";
					// check for change in address value due to sw about to happen or not
					// auto pos = changed_addresses.find(new_address);
					// if (pos == changed_addresses.end()){
						registerValueChange[reg1] = 1;
						ID -> occupied = 0;
						EX -> occupied = 1;
						EX -> instruct = ID -> instruct;
						// if (flag == 1){
						// 	flag = 0;
						// 	run_EX();
						// }
						return;
					// }
				}else flag = 1;
				
			}else if ((ID -> instruct)[0] == "sw"){
				std::string reg2 = get_register((ID -> instruct)[2]);
				std::string reg1 = (ID -> instruct)[1];
				
				if (registerValueChange[reg2] == 0){
					if (registerValueChange[reg1] == 0){
						int new_address = locateAddress((ID->instruct)[2]);
						changed_addresses.insert(new_address);
						ID -> occupied = 0;
						EX -> occupied = 1;
						EX -> instruct = ID -> instruct;
					}
					
					// if (flag == 1){
					// 	flag = 0;
					// 	run_EX();
					// }
				}else flag = 1;
			}else if ((ID -> instruct)[0] == "j"){
				// if (PCcurr != commands.size()){
				// 	IF -> nop = 1;
				// }
				
				// int old_PC = PCcurr;
				
				instructions[(ID -> instruct)[0]](*this, (ID -> instruct)[1], (ID -> instruct)[2], (ID -> instruct)[3]);
				// force_stall = 1;
				// if (old_PC == PCcurr) IF -> nop = 0;
				stall--;
				
				ID -> occupied = 0;
				EX -> occupied = 1;
				EX -> instruct = ID -> instruct;
				EX -> nop = 1;
		   }else if (((ID -> instruct)[0] == "beq") || ((ID -> instruct)[0] == "bne")){
				std::string reg2 = (ID -> instruct)[1];
				std::string reg3 = (ID -> instruct)[2];
				if ((registerValueChange[reg2] == 0) && (registerValueChange[reg3] == 0)){
					instructions[(ID -> instruct)[0]](*this, (ID -> instruct)[1], (ID -> instruct)[2], (ID -> instruct)[3]);
					ID -> occupied = 0;
					EX -> occupied = 1;
					EX -> instruct = ID -> instruct;
					EX -> nop = 1;
					// IF -> nop = 1;
					// stall = 1;
					return;
				}
			}
		}
		
		return;
	}
	
	void run_EX(){
		// std::cout << " EX ran " << std::endl;
		
		
		if (MEM -> occupied == 0){
			if (EX -> nop == 1){
				// std::cout << "aanee" ;
				if (((EX -> instruct)[0] == "bne") || ((EX -> instruct)[0] == "beq")) stall--;
				
				EX -> nop = 0;
				MEM -> nop = 1;
				MEM -> occupied = 1;
				EX -> occupied = 0;
				MEM -> instruct = EX -> instruct;
				
				return;
			}
			
			
			std::string instr = (EX -> instruct)[0];
			
			
			if ((instr!="lw") && (instr!="sw")) {
				choice = 1;
				// std::cout << "screams yes yes yes" << choice << std::endl;
				std::string reg1 = (EX-> instruct)[1];
				// std::cout << reg1;
		        registerValueChange[reg1] = 0;
		        // std::cout << "ajeeb hai" << registers[2] <<std::endl;
		        // std::cout << (EX-> instruct)[0] << std::endl;
		        // std::cout << (EX-> instruct)[1] << std::endl;
		        // std::cout << (EX-> instruct)[2] << std::endl;
		        // std::cout << (EX-> instruct)[3] << std::endl;

		        instructions[(EX-> instruct)[0]](*this, (EX -> instruct)[1], (EX -> instruct)[2], (EX -> instruct)[3]);
		        // std::cout << "ajeeb hai pt 2" << registers[2] <<std::endl;
				
			}
			MEM -> occupied = 1;
			EX -> occupied = 0;
			MEM -> instruct = EX -> instruct;
		}
		return;
	}
	
	void run_MEM(){
		// std::cout << " MEM ran " << std::endl;
		// std::cout << "aanyaaaa " << MEM-> nop << std::endl;
		if (MEM -> nop == 1){
			// std::cout << "ithe aa gaye ke";
			if (WB -> occupied == 0) {
				// std::cout << "heluuuu" << std::endl;
				WB -> occupied = 1;
				WB -> instruct = MEM -> instruct;
				MEM -> occupied = 0;
				WB -> nop = 1;
				MEM -> nop = 0;
			    return;
				}
			}
		
		if ((MEM -> instruct)[0] == "sw"){
			// if sw then the instruction executes here and updates the memory
			int new_address = locateAddress((MEM->instruct)[2]);
			// auto pos = changed_addresses.find(new_address);
			// changed_addresses.erase(pos);
			// mem_writes = new_address;
			// std::cout << "yes yes yes yes" << std::endl;

			instructions[(MEM -> instruct)[0]](*this, (MEM -> instruct)[1], (MEM -> instruct)[2], (MEM -> instruct)[3]);
            // std::cout << "sw hai and"  << values.size() << mem_writes.size()<< std::endl;
			values.push_back(data[new_address]);
			// std::cout << "values" << values[0] << std::endl;
			mem_writes.push_back(new_address);
			// std::cout << "mem_writes" << mem_writes[0]<< std::endl;
		}
		
		if (WB -> occupied == 0){
			MEM -> occupied = 0;
			// std::cout << "aana chahiye" << std::endl;
			WB -> occupied = 1; 
			std::string instr = (MEM -> instruct)[0] ;
			if (instr=="lw") {
				choice = 1;
				// std::cout << "nahi aana chahiye" << std::endl; 
				std::string reg1 = (MEM-> instruct)[1];
		        registerValueChange[reg1] = 0;
		        instructions[(MEM-> instruct)[0]](*this, (MEM -> instruct)[1], (MEM -> instruct)[2], (MEM -> instruct)[3]);
		    } 
		        
			WB -> instruct = MEM -> instruct;
			
			if ((MEM -> instruct)[0] == "sw") WB -> nop = 1;
		}
		
		// std::cout << "mem fin";
		return;
	}
	
	void run_WB(){
		// std::cout << " WB ran " << std::endl;
		if (mem_writes.size() > 0) mem_writes.pop_back();
		if (values.size() > 0) values.pop_back();
			
		if (WB -> nop == 1){
			// std::cout << "ee ka";
			// std::cout << "mem_writes" << mem_writes.size() << std::endl;
			WB -> occupied = 0;
			WB -> nop = 0;
			return;
		}
		
		// std::string reg1 = (WB-> instruct)[1];
		// registerValueChange[reg1] = 0;
		choice = 0;
		// std::cout << "init val is" << registers[1] << std::endl;
		// std::cout << (WB -> instruct)[0] << " "  << (WB -> instruct)[1];
		if (mem_writes.size() > 0) mem_writes.pop_back();
		if (values.size() > 0) values.pop_back();

		instructions[(WB -> instruct)[0]](*this, (WB -> instruct)[1], (WB -> instruct)[2], (WB -> instruct)[3]);

        // std::cout << "final val is" << registers[1] << std::endl;
		WB -> occupied = 0;
		return;
	}
	
	void executeCommandsPipelined()
	{
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;
		int half = 0;
		
		// std::vector<std::string> &command = commands[PCcurr];
		printRegisters(clockCycles);
		std::cout << mem_writes.size() << " ";
		for (int i = 0; i < mem_writes.size(); i++) std::cout << mem_writes[i] << " " << values[i];
		std::cout << std::endl;
		clockCycles = 1;

		// std::cout << "the number of commands is " << commands.size() << std::endl;
		
		start_IF(); //start running
		// std::cout << "starting " << IF -> occupied << " ";
		//successfully started
		
		while (1) {
			// std::cout << "current " << PCcurr << " ";
			if (half == 0){
				if (WB -> occupied == 1) run_WB(); 
			}else{
				if (MEM -> occupied == 1) run_MEM();
				if (EX -> occupied == 1) run_EX();
				if (ID -> occupied == 1) run_ID();
				if (IF -> occupied == 1) run_IF();
				if (PCcurr < commands.size()) start_IF();
			}
			
			if (half == 0) half = 1;
			else{
				printRegisters(clockCycles);
				std::cout << mem_writes.size() << " ";
				for (int i = 0; i < mem_writes.size(); i++) std::cout << mem_writes[i] << " " << values[i];
				std::cout << std::endl;
				
				half = 0;
				clockCycles++;

			}	
			
			if ((WB -> occupied == 0) && (MEM -> occupied == 0) && (EX -> occupied == 0) && (ID -> occupied == 0) && (IF -> occupied == 0)){
				break;
			}
		}
		printRegisters(clockCycles);
		std::cout << mem_writes.size() << " ";
		for (int i = 0; i < mem_writes.size(); i++) std::cout << mem_writes[i] << " " << values[i];
		std::cout << std::endl;
	}


	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		// std::cout << "Cycle number: " << clockCycle << '\n';
		std::cout << std::dec;
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout << std::dec << '\n';
	}
};

#endif