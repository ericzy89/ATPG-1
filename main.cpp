#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include "tokens.h"
#include "statements.h"
#include "checkpoints.h"
#include "netlist.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "You should provide a file name." << std::endl;
        return -1;
    }

	std::string evl_file = argv[1];
	std::cout << "Successfully wrote the following files: " << std::endl;
	evl_modules modules;
	
	if (!parse_evl_file(argv[1], modules)) {
		return -1;
	}

	if (!make_checkpoints(argv[1], modules)) {
		return -1;
	}

	//netlist nl;
	//if (!nl.create(modules.begin()->c_wires, modules.begin()->c_components)) {
	//	return -1;
	//}

	//std::string n1_file = std::string(evl_file + ".netlist");
	//nl.save(n1_file);

	//nl.file_name = evl_file;
	//nl.simulate(1000);
	
	
	return 0;
}

