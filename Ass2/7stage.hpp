#ifndef __7STAGE_HPP__
#define __7STAGE_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>

struct stage
	{
		// std::string stage_name;
		std::vector<std::string> instruct;
		int nop = 0;
		int occupied = 0;
	};

struct MIPS_Architecture
{
	int registers[32] = {0}; // an array of 32 values all initialised to 0
	int PCcurr = 0; // Current PC Counter set to 0
	int PCnext; // Next PC counter unitiated
	
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
	std::unordered_map<std::string, int> registerMap, registerValueChange, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;
	int start_from_here = 0;
	std::vector<int> mem_writes;
	std::vector<int> values;
	
	int ID_visited = 0;
	int flag = 0;
	int force_stall = 0;
	int stall = 0;
	int another = 0;
	int port = 0;
	int operated = 0;
	
	struct stage *N = new stage;
	struct stage *IF1 = new stage;
	struct stage *IF2 = new stage;
	struct stage *ID1 = new stage;
	struct stage *ID2 = new stage;
	struct stage *RR = new stage;
	struct stage *EX_mem = new stage;
	struct stage *EX_reg = new stage;
	struct stage *MEM1 = new stage;
	struct stage *MEM2= new stage;
	struct stage *WB = new stage;
	struct stage *WB2 = new stage;
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

	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};
		
		for (int i = 0; i < 32; ++i){
			registerMap["$" + std::to_string(i)] = i;
			registerValueChange["$" + std::to_string(i)] = 0;
		}
		
		registerMap["$zero"] = 0; //$zero mapped to 0
		registerMap["$at"] = 1; 
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		
		registerValueChange["$zero"] = 0;
		registerValueChange["$at"] = 0; 
		registerValueChange["$v0"] = 0;
		registerValueChange["$v1"] = 0;
		
		for (int i = 0; i < 4; ++i){
			registerMap["$a" + std::to_string(i)] = i + 4; //$a0 is 4 .. $a3 is 7
			registerValueChange["$a" + std::to_string(i)] = 0;
		}
		
		for (int i = 0; i < 8; ++i){
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
			registerValueChange["$t" + std::to_string(i)] = 0, registerValueChange["$s" + std::to_string(i)] = 0;
		}
		
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;
		
		registerValueChange["$t8"] = 0;
		registerValueChange["$t9"] = 0;
		registerValueChange["$k0"] = 0;
		registerValueChange["$k1"] = 0;
		registerValueChange["$gp"] = 0;
		registerValueChange["$sp"] = 0;
		registerValueChange["$s8"] = 0;
		registerValueChange["$ra"] = 0;

		constructCommands(file); //parses and stores every command in the commands vector
		commandCount.assign(commands.size(), 0); //stores how many times each command is run
	}

	// perform add operation
	int add(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });		
	} //[] is the  lambda operator in cpp.

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
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = operation(registers[registerMap[r2]], registers[registerMap[r3]]);
		// PCnext = PCcurr + 1;
		return 0;
	}

	// perform the beq operation
	int beq(std::string r1, std::string r2, std::string label)
	{
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
		PCcurr = comp(registers[registerMap[r1]], registers[registerMap[r2]]) ? address[label] : PCcurr ;

		return 0;
	}

	// implements slt operation
	int slt(std::string r1, std::string r2, std::string r3)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = registers[registerMap[r2]] < registers[registerMap[r3]];
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
		registers[registerMap[r]] = data[address];
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
				if (!checkRegister(reg)){
					return -3;
				}
				int address = registers[registerMap[reg]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				{
					return -3;
				}
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
			registers[registerMap[r1]] = registers[registerMap[r2]] + stoi(num);
			PCnext = PCcurr + 1;
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

	std::string get_register(std::string location){
		int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
		std::string reg = location.substr(lparen + 1);
		reg.pop_back();
		return reg;
	}
	
	void start_IF(){
		if (stall == 1){
			return;
		}
		if (IF1 -> occupied == 0){
			// std::cout << "PC taken in IF is " << PCcurr << " ";
			IF1 -> instruct = commands[PCcurr];
			IF1 -> occupied = 1;
			PCcurr++;
			// so that same instruction runs again (case of curr)
		}
	}
	
	//whichever is occupied we run it
	void run_IF1(){
		// std::cout << "IF1 ran " << std::endl;
		// std::cout << "stall is " << stall;
		
		if (IF1 -> nop == 1){
			if (IF2 -> occupied == 0){
				IF2 -> occupied = 1;
				IF1 -> occupied = 0;
				IF2 -> nop = 1;
				IF1 -> nop = 0;
				IF2 -> instruct = IF1 -> instruct;
				return;
			}
		}
		
		if (IF2 -> occupied == 0){
			if ((IF1 -> instruct)[0] == "bne" || (IF1 -> instruct)[0] == "beq" || ((IF1 -> instruct)[0] == "j")) stall++;
			IF2 -> occupied = 1;
			IF1 -> occupied = 0;
			IF2 -> instruct = IF1 -> instruct;
			return;
		}
	}
	
	void run_IF2(){
		// std::cout << " IF2 ran " << std::endl;
		if (IF2 -> nop == 1){
			if (ID1 -> occupied == 0){
				ID1 -> occupied = 1;
				IF2 -> occupied = 0;
				IF2 -> nop = 0;
				ID1 -> nop = 1;
				ID1 -> instruct = IF2 -> instruct;
				return;
			}
		}
		
		if (ID1 -> occupied == 0){
			ID1 -> occupied = 1;
			IF2 -> occupied = 0;
			ID1 -> instruct = IF2 -> instruct;	
			return;
		}
	}
	
	void run_ID1(){
		// std::cout << " ID1 ran " << std::endl;
		if (ID1 -> nop == 1){
			if (ID2 -> occupied == 0){
				ID2 -> occupied = 1;
				ID1 -> occupied = 0;
				ID1 -> nop = 0;
				ID2 -> nop = 1;
				ID2 -> instruct = ID1 -> instruct;
				return;
			}
		}
		
		if (ID2 -> occupied == 0){
			ID2 -> occupied = 1;
			ID1 -> occupied = 0;
			ID2 -> instruct = ID1 -> instruct;	
			return;
		}
	}
	
	void run_ID2(){
		// std::cout << " ID2 ran " << std::endl; 
		
		if (ID2 -> nop == 1){
			if (RR -> occupied == 0){
				ID2 -> occupied = 0;
				RR -> occupied = 1;
				RR -> instruct = ID2 -> instruct;
				RR -> nop = 1;
				ID2 -> nop = 0;
				return;
			}
		}
		
		if ((ID2 -> instruct)[0] == "j"){
			if (operated == 0) {
				instructions[(ID2 -> instruct)[0]](*this, (ID2 -> instruct)[1], (ID2 -> instruct)[2], (ID2 -> instruct)[3]);

				stall--;
				operated = 1;
			}
			
			if (RR -> occupied == 0){
				ID2 -> occupied = 0;
				RR -> occupied = 1;
				RR -> instruct = ID2 -> instruct;
				RR -> nop = 1;
				operated = 0;
			}
			return;
		}
		
		
		if (RR -> occupied == 0){
			if ((ID2 -> instruct)[0] == "addi"){
				std::string reg1 = (ID2 -> instruct)[1];
				std::string reg2 = (ID2 -> instruct)[2];
				
				if (registerValueChange[reg2] == 0){
					registerValueChange[reg1] = 1;
					ID2 -> occupied = 0;
					RR -> occupied = 1;
					RR -> instruct = ID2 -> instruct;
					return;
				}
			}else if (((ID2 -> instruct)[0] == "add") || ((ID2 -> instruct)[0] == "sub") || ((ID2 -> instruct)[0] == "mul") || ((ID2 -> instruct)[0] == "slt") ){
				std::string reg1 = (ID2 -> instruct)[1];
				std::string reg2 = (ID2 -> instruct)[2];
				std::string reg3 = (ID2 -> instruct)[3];
				
				if ((registerValueChange[reg2] == 0) && (registerValueChange[reg3] == 0)){
					registerValueChange[reg1] = 1;
					ID2 -> occupied = 0;
					RR -> occupied = 1;
					RR -> instruct = ID2 -> instruct;
					return;
				}
			}else if ((ID2 -> instruct)[0] == "lw"){
				std::string reg2 = get_register((ID2 -> instruct)[2]);
				std::string reg1 = (ID2 -> instruct)[1];
				int new_address = locateAddress((ID2 -> instruct)[2]);
				
				if (registerValueChange[reg2] == 0){
						registerValueChange[reg1] = 1;
						ID2 -> occupied = 0;
						RR -> occupied = 1;
						RR -> instruct = ID2 -> instruct;
						return;
				}
				
			}else if ((ID2 -> instruct)[0] == "sw"){
				std::string reg2 = get_register((ID2 -> instruct)[2]);
				std::string reg1 = (ID2 -> instruct)[1];
				
				if (registerValueChange[reg2] == 0){
					if (registerValueChange[reg1] == 0){
						int new_address = locateAddress((ID2 -> instruct)[2]);
						ID2 -> occupied = 0;
						RR -> occupied = 1;
						RR -> instruct = ID2 -> instruct;
					}
				}
			}else if ((ID2 -> instruct)[0] == "j"){
				stall--;
				instructions[(ID2 -> instruct)[0]](*this, (ID2 -> instruct)[1], (ID2 -> instruct)[2], (ID2 -> instruct)[3]);
				
				ID2 -> occupied = 0;
				RR -> occupied = 1;
				RR -> instruct = ID2 -> instruct;
				RR -> nop = 1;
			}else if (((ID2 -> instruct)[0] == "beq") || ((ID2 -> instruct)[0] == "bne")){
				std::string reg2 = (ID2 -> instruct)[1];
				std::string reg3 = (ID2 -> instruct)[2];
				
				if ((registerValueChange[reg2] == 0) && (registerValueChange[reg3] == 0)){
					instructions[(ID2 -> instruct)[0]](*this, (ID2 -> instruct)[1], (ID2 -> instruct)[2], (ID2 -> instruct)[3]);
					// std::cout << " updated PC " << PCcurr;
					ID2 -> occupied = 0;
					RR -> occupied = 1;
					RR -> instruct = ID2 -> instruct;
					RR -> nop = 1;
					return;
				}
			}
		}
		
		return;
	}
	
	void run_RR(){
		// std::cout << " RR ran " << std::endl;
		
		if (((RR -> instruct)[0] == "lw") || ((RR -> instruct)[0] == "sw")){
			if (RR -> nop == 1){
				if (EX_mem -> occupied == 0){
					EX_mem -> nop = 1;
					RR -> nop = 0;
					EX_mem -> occupied = 1;
					RR -> occupied = 0;
					EX_mem -> instruct = RR -> instruct;
				}
			}
			
			if (EX_mem -> occupied == 0){
				EX_mem -> occupied = 1;
				RR -> occupied = 0;
				EX_mem -> instruct = RR -> instruct;
			}
			
		}else{
			if (RR -> nop == 1){
				if (EX_reg -> occupied == 0){
					EX_reg -> nop = 1;
					RR -> nop = 0;
					EX_reg -> occupied = 1;
					RR -> occupied = 0;
					EX_reg -> instruct = RR -> instruct;
				}
			}
			if (EX_reg -> occupied == 0){
				EX_reg -> occupied = 1;
				RR -> occupied = 0;
				EX_reg -> instruct = RR -> instruct;
			}
		}
	}
	
	void run_EX_reg(){
		// std::cout << " EX_reg ran " << std::endl;
		

		if (flag == 0){
				if (EX_reg -> nop == 1){
					EX_reg -> nop = 0;
					WB2 -> occupied = 1;
					EX_reg -> occupied = 0;
					WB2 -> nop = 1;
				}	
				
				if (((EX_reg -> instruct)[0] == "bne") || ((EX_reg -> instruct)[0] == "beq")) stall--;
				WB2 -> occupied = 1;
				EX_reg -> occupied = 0;
				WB2 -> instruct = EX_reg -> instruct;
		}
		return;
	}
	
	void run_EX_mem(){
		// std::cout << " EX_mem ran " << std::endl;
		flag++;
		
		if (MEM1-> occupied == 0){
			if (EX_mem -> nop == 1){
				EX_mem -> nop = 0;
				MEM1 -> nop = 1;
			}
			
			MEM1 -> occupied = 1;
			EX_mem -> occupied = 0;
			MEM1 -> instruct = EX_mem -> instruct;
		}
		return;
	}
	
	void run_MEM1(){
		// std::cout << " MEM1 ran " << std::endl;
		if (MEM1 -> nop == 1){
			if (MEM2 -> occupied == 0){
				MEM2 -> occupied = 1;
				MEM1 -> occupied = 0;
				MEM1 -> nop = 0;
				MEM2 -> nop = 1;
				MEM2 -> instruct = MEM1 -> instruct;
				return;
			}
		}
		
		if (MEM2 -> occupied == 0){
			MEM2 -> occupied = 1;
			MEM1 -> occupied = 0;
			MEM2 -> instruct = MEM1 -> instruct;	
			return;
		}
	}
	
	void run_MEM2(){
		flag--;
		// std::cout << " MEM2 ran " << std::endl;
		if (MEM2 -> nop == 1){
			if (WB -> occupied == 0){
				WB -> occupied = 1;
				WB -> instruct = MEM2 -> instruct;
				MEM2 -> occupied = 0;
				WB -> nop = 1;
				MEM2 -> nop = 0;
			}
			return;
		}
		
		if ((MEM2 -> instruct)[0] == "sw"){
			int new_address = locateAddress((MEM2 -> instruct)[2]);
			instructions[(MEM2 -> instruct)[0]](*this, (MEM2 -> instruct)[1], (MEM2 -> instruct)[2], (MEM2 -> instruct)[3]);
			mem_writes.push_back(new_address);
			values.push_back(data[new_address]);
		}
		
		if (WB -> occupied == 0){
			MEM2 -> occupied = 0;
			WB -> occupied = 1;
			WB -> instruct = MEM2 -> instruct;
			
			if ((MEM2 -> instruct)[0] == "sw") WB -> nop = 1;
		}
		return;
	}
	
	void run_WB(){
		if (mem_writes.size() > 0) mem_writes.pop_back();
		if (values.size() > 0) values.pop_back();
		
		if ((WB -> instruct)[0] != "sw" ) port = 1;
		
		// std::cout << " WB ran " << std::endl;
		if (WB -> nop == 1){
			WB -> occupied = 0;
			WB -> nop = 0;
			return;
		}
		
		std::string reg1 = (WB-> instruct)[1];
		registerValueChange[reg1] = 0;
		
		instructions[(WB -> instruct)[0]](*this, (WB -> instruct)[1], (WB -> instruct)[2], (WB -> instruct)[3]);
		WB -> occupied = 0;
		return;
	}
	
	void run_WB2(){
		if (mem_writes.size() > 0) mem_writes.pop_back();
		if (values.size() > 0) values.pop_back();
		
		// std::cout << " WB2 ran " << std::endl;
		if (WB2 -> nop == 1){
			WB2 -> occupied = 0;
			WB2 -> nop = 0;
			return;
		}
		if (port == 0) instructions[(WB2 -> instruct)[0]](*this, (WB2 -> instruct)[1], (WB2 -> instruct)[2], (WB2 -> instruct)[3]);
		else return;
		
		std::string reg1 = (WB2 -> instruct)[1];
		registerValueChange[reg1] = 0;
		
		
		WB2 -> occupied = 0;
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

		printRegisters(clockCycles);
		std::cout << mem_writes.size() << " ";
		for (int i = 0; i < mem_writes.size(); i++) std::cout << mem_writes[i] << " " << values[i];
		std::cout << std::endl;
	
		clockCycles = 1;
		
		start_IF(); //start running
		// std::cout << "starting " << IF1 -> occupied << " ";
		
		while (1){
			if (half == 0){
				if (WB -> occupied == 1) run_WB();
				if (WB2 -> occupied == 1) run_WB2();
				port = 0;
			}else{
				if (MEM2 -> occupied == 1) run_MEM2();
				if (MEM1 -> occupied == 1) run_MEM1();
				if (EX_mem -> occupied == 1) run_EX_mem();
				if (EX_reg -> occupied == 1) run_EX_reg();
				if (RR -> occupied == 1) run_RR();
				if (ID2 -> occupied == 1) run_ID2();
				if (ID1 -> occupied == 1) run_ID1();
				if (IF2 -> occupied == 1) run_IF2();
				if (IF1 -> occupied == 1) run_IF1();
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
			
			if ((WB -> occupied == 0) && (WB2 -> occupied == 0) && (MEM2 -> occupied == 0) && (MEM1 -> occupied == 0) && (RR -> occupied == 0) && (EX_mem -> occupied == 0) && (EX_reg -> occupied == 0) && (ID2 -> occupied == 0) && (ID1 -> occupied == 0) && (IF2 -> occupied == 0) && (IF1 -> occupied == 0)){
				break;
			}
		}
		printRegisters(clockCycles);
		// std::cout << std::endl;
		std::cout << mem_writes.size() << " ";
		for (int i = 0; i < mem_writes.size(); i++) std::cout << mem_writes[i] << " " << values[i];
		std::cout << std::endl;
	}

	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		// std::cout << "Cycle number: " << clockCycle << '\n';
		std::cout	<< std::dec;
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout << std::dec << '\n';
	}
};

#endif
