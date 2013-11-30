#include <ctype.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <assert.h>
#include "tokens.h"
#include "statements.h"
#include "checkpoints.h"

void display_checkpoints(std::ostream &out, const evl_modules &modules) {
	for (evl_modules::const_iterator it1 = modules.begin(); it1 != modules.end(); ++it1) {
		out << "Wires to check:\n" << std::endl;
		for(evl_wires::const_iterator it2 = it1->c_wires.begin(); it2 != it1->c_wires.end(); ++it2) {
			if(it2->second > 3){
				out << "wire " << it2->first << std::endl;
			}
		}
	}
}
bool store_checkpoints_to_file(std::string file_name, evl_modules &modules) {
	std::ofstream output_file(file_name.c_str());
	if(!output_file) {
		std::cerr << "I can't write Module File." << std::endl;
		return false;
	}
	display_checkpoints(output_file, modules);

	std::cout << file_name << std::endl;
	return true;
}

bool make_checkpoints(std::string file_name, evl_modules &modules) {
	//display_module(std::cout,modules);
	if (!store_checkpoints_to_file(file_name + ".checkpoints", modules)) {
		return false;
	}
	return true;
}