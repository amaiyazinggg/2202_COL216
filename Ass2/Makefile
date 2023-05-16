compile : 5stage.cpp 5stage_new.hpp 5stage_bypass.cpp 5stage_bypass.hpp 79stage.cpp 7stage.hpp 79stage_bypass.cpp 7stage_bypass.hpp
	g++ 5stage.cpp 5stage_new.hpp -o 5stage 
	g++ 5stage_bypass.cpp 5stage_bypass.hpp -o 5stagebp 
	g++ 79stage.cpp 7stage.hpp -o 7stage
	g++ 79stage_bypass.cpp 7stage_bypass.hpp -o 7stagebp
	
run_5stage : 
	./5stage input.asm
	
run_5stage_bypass : 
	./5stagebp input.asm
	
run_79stage : 
	./7stage input.asm
	
run_79stage_bypass : 
	./7stagebp input.asm