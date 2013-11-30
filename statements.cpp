#include <ctype.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <assert.h>
#include "tokens.h"
#include "statements.h"


bool move_comp_to_statement(evl_tokens &statement_tokens, evl_tokens &tokens) {
	assert(statement_tokens.empty());
	assert(!tokens.empty());

	for (; !tokens.empty();) {
		statement_tokens.push_back(tokens.front());
		tokens.erase(tokens.begin());
		if (statement_tokens.back().str == ")")
			break;		//exit if the ending ")" is found
	}
	if (statement_tokens.back().str != ")") {
		std::cerr << "Look for ')' but reach the end of file" << std::endl;
		return false;
	}
	return true;
}
bool move_wire_to_statement(evl_tokens &statement_tokens, evl_tokens &tokens) {
	assert(statement_tokens.empty());
	assert(!tokens.empty());

	for (; !tokens.empty();) {
		statement_tokens.push_back(tokens.front());
		tokens.erase(tokens.begin());
		if (statement_tokens.back().str == "=")
			break;		//exit if the ending "=" is found
	}
	if (statement_tokens.back().str != "=") {
		std::cerr << "Look for '=' but reach the end of file" << std::endl;
		return false;
	}
	return true;
}
bool group_tokens_into_statements(evl_statements &statements, evl_tokens &tokens) { 
	assert(statements.empty());

	for(; !tokens.empty();) {	//generates one statement per iteration
		evl_token token = tokens.front();

		if (token.type != evl_token::NAME) {
			std::cerr << "Need a NAME token but found '" << token.str << "' on line" << token.line_no << std::endl;
			return false;
		}

		if ((token.str == "INPUT") || (token.str == "OUTPUT") || (token.str == "AND") ||
			(token.str == "NAND") || (token.str == "OR") || (token.str == "NOR") ||
			(token.str == "DFF") || (token.str == "NOT")) {
			
			evl_statement component;
			component.type = evl_statement::COMPONENT;
			

			if(!move_comp_to_statement(component.tokens, tokens)) {
				return false;
			}

			//INPUT and OUTPUT are both components and wires;
			if ((token.str == "INPUT") || (token.str == "OUTPUT")) {
				evl_statement wire;
				wire.type = evl_statement::WIRE;
				wire.tokens = component.tokens;
				statements.push_back(wire);
			}
			statements.push_back(component);
		}
		else {
			evl_statement wire;
			wire.type = evl_statement::WIRE;

			if(!move_wire_to_statement(wire.tokens, tokens)) {
				return false;
			}
			statements.push_back(wire);
		}
	}
	return true;
}

//Re-order statements so order is MODULE,WIRES,COMPONENTS,ENDMODULE,...repeat
bool reorder_statements_list (evl_statements::const_iterator m_itr, evl_statements &statements){
        
	m_itr = statements.begin();

	for(evl_statements::const_iterator it = statements.begin(); it != statements.end(); ++it) {
		if (it->type == evl_statement::WIRE) {
			statements.insert(++m_itr,*it);
			m_itr--;
			it = statements.erase(it);
			//--it;
		}
	}        
	return true;
}
void display_statements(std::ostream &out, const evl_statements &statements) {
	int count = 1;
	for (evl_statements::const_iterator iter = statements.begin(); iter != statements.end(); ++iter) {
		if (iter->type == evl_statement::MODULE) {
			out << "STATEMENT " << count << " MODULE, " << iter->tokens.size() << " tokens" << std::endl;
		}
		else if (iter->type == evl_statement::WIRE) {
			out << "STATEMENT " << count << " WIRE, " << iter->tokens.size() << " tokens" << std::endl;
		}
		else if (iter->type == evl_statement::COMPONENT) {
			out << "STATEMENT " << count << " COMPONENT, " << iter->tokens.size() << " tokens" << std::endl;
		}
		else if (iter->type == evl_statement::ENDMODULE) {
			out << "STATEMENT " << count << " ENDMODULE" << std::endl;
		}
		else {
			out << "ERROR" << std::endl;
		}
		count++;
	}
}
bool store_statements_to_file(std::string file_name, const evl_statements &statements) {

	std::ofstream output_file(file_name.c_str());
	if(!output_file) {
		std::cerr << "I can't write Statment file." << std::endl;
		return false;
	}
	display_statements(output_file, statements);
	
	std::cout << file_name << std::endl;

	return true;
}
//Module name and module number stored in a std::map
int i = 0;
bool process_statements(evl_modules &modules, evl_statement &s) {
	//assert(s.type == evl_statement::MODULE);

	enum state_type {INIT, NAME, SEMI_CHECK, CHK_STMT, WIRE, COMPONENT, ENDMOD, DONE};

	state_type state;

	if (s.type == evl_statement::WIRE) {
		state = WIRE;
	}
	else if (s.type == evl_statement::COMPONENT) {
		state = COMPONENT;
	}

	if(i == 0) {
		evl_module mod;
		mod.name = "top";
		modules.push_back(mod);
		i++;
	}
	for (; !s.tokens.empty() && (state != DONE); s.tokens.pop_front()) {
		evl_token t = s.tokens.front();

		if (state == WIRE) {
			evl_wires wires;
			if (!process_wire_statements(wires,s)) {
					return false;
			}
			else {
				modules.back().c_wires.insert(wires.begin(), wires.end());
				state = DONE;
				break;	
			}
		}
		else if (state == COMPONENT) {
			evl_components comp;
			if (!process_component_statements(comp,s,modules.back().c_wires,modules.back().c_checkpoints)) {
				return false;
			}
			else {
				modules.back().c_components.splice(modules.back().c_components.end(),comp);
				state = DONE;
				break;
			}
		}
		else {
			//state = DONE;
			assert(false); //never should get here
		}
	}
	if (!s.tokens.empty() || (state != DONE)) {
		std::cerr << "something wrong with the statement in mod" << std::endl;
		return false;
	}
	return true;
}
bool process_wire_statements(evl_wires &wires, evl_statement &s) {
	assert(s.type == evl_statement::WIRE);
	
	enum state_type {INIT, WIRE,IO, WIRES, WIRE_NAME, BUS, BUS_MSB, BUS_COLON, BUS_LSB, BUS_DONE, DONE};

	state_type state = INIT;
	int bus_width = 1;
	for (; !s.tokens.empty() && (state != DONE); s.tokens.pop_front()) {
		evl_token t = s.tokens.front();
		if (state == INIT) {
			
			if ((t.str == "INPUT") || (t.str == "OUTPUT")) {
				state = IO;
			}
			else {
				if (t.type == evl_token::NAME) {
					evl_wires::iterator it = wires.find(t.str);
					if(it != wires.end()) {
						std::cerr << "Wire '" << t.str << "' on line " << t.line_no << " is already defined" << std::endl;
						return false;
					}
					wires.insert(std::make_pair(t.str, bus_width));
					state = WIRE_NAME;
				}
			}
		}
		else if (state == IO) {
			if (t.type == evl_token::SINGLE) {
				if(t.str != "("){
					std::cerr << "Need a '(' but found '" << t.str << "' on line" << t.line_no << std::endl;
					return false;
				}
				state = WIRE;
			}
			else {
				std::cerr << "Need Input or Output designation but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;		
			}
		}
		else if (state == WIRE) {
			if (t.type == evl_token::NAME) {
				evl_wires::iterator it = wires.find(t.str);
				if(it != wires.end()) {
					std::cerr << "Wire '" << t.str << "' on line " << t.line_no << " is already defined" << std::endl;
					return false;
				}
				wires.insert(std::make_pair(t.str, bus_width));
				state = WIRE_NAME;
			}
			else {
				std::cerr << "Need 'wire' but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == WIRE_NAME) {
			if ((t.str == ")") || (t.str == "=")) {
				state = DONE;
			}
			else {
				std::cerr << "Need ',' or ';' but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else {
			assert(false); //never should get here
		}
	}
	if (!s.tokens.empty() || (state != DONE)) {
		std::cerr << "something wrong with the statement in wires" << std::endl;
		return false;
	}

	return true;
}
bool process_component_statements(evl_components &components, evl_statement &s, evl_wires &wires, evl_checkpoints &check) {
	//assert(s.type == evl_statement::COMPONENT);

	enum state_type {INIT, TYPE, NAME, PINS, PIN_NAME, PINS_DONE, BUS, BUS_MSB, BUS_COLON, BUS_LSB, BUS_DONE, DONE};
	
	std::string comp_type = "";
	std::string comp_name = "NONE";
	std::string pin_name = "";
	int bus_msb = -1;
	int bus_lsb = -1;
	int count = 0;
	int count_2;
	state_type state = INIT;

	for(; !s.tokens.empty() && (state != DONE); s.tokens.pop_front()){
		evl_token t = s.tokens.front();
		
		if (state == INIT) {
			if(t.type == evl_token::NAME) {
				comp_type = t.str;
				state = TYPE;
			}
		    else {
				state = TYPE;				
			}
		}
		else if (state == TYPE){
			if (t.type == evl_token::SINGLE && t.str == "(") {
				state = PINS;
			}
			else {
				std::cerr << "Need a NAME or a '(' but found '" << t.str << "' at line " << t.line_no << std::endl;
				return false;
			}
			evl_component component;
			component.type = comp_type;
			component.name = comp_name;
			components.push_back(component);
			
			if(comp_type == "OUTPUT") {
				count_2 = 10;
			}
			else
				count_2 = 0;

		}
		else if (state == PINS) {
			//int count = 0;
			if (t.type == evl_token::NAME) {
				evl_wires::iterator it = wires.find(t.str);
				if (it == wires.end()) {
					std::cerr << "Wire '" << t.str << "' on line " << t.line_no << " is not defined" << std::endl;
					return false;
				}
				count = it->second + count_2;
				it->second = ++count;
				pin_name = t.str;
			}
			state = PIN_NAME;			
		}
		else if (state == PIN_NAME) {
			bus_msb = -1;
			bus_lsb = -1;
			//int count = 0;
			if (t.str == ")"){
				evl_wires::iterator it = wires.find(pin_name);
				count = it->second + count_2;
				it->second = ++count; 
				//if (it->second >= 2) {
				//	bus_msb = it->second-1;
				//	bus_lsb = 0;
				//}
				evl_pin pins;
				pins.lsb = bus_lsb;
				pins.msb = bus_msb;
				pins.pin_name = pin_name;
							
				components.back().comp_pins.push_back(pins);
				
				state = DONE;
			}
			else if (t.str == ","){
				evl_wires::iterator it = wires.find(pin_name);
				//if (it->second >= 2) {
				//	bus_msb = it->second-1;
				//	bus_lsb = 0;
				//}
				count = it->second + count_2;
				it->second = ++count; 
				evl_pin pins;			
				pins.lsb = bus_lsb;
				pins.msb = bus_msb;
				pins.pin_name = pin_name;	

				components.back().comp_pins.push_back(pins);
				
				state = PINS;
			}
			else { 
				std::cerr << "Need ',' or ')' or '[' but found '" << t.str << "' on line " << t.line_no <<std::endl;
				return false;
			}
		}
		else {
			assert(false); //never should get here
		}
	}
	if (!s.tokens.empty() || (state != DONE)) {
		std::cerr << "something wrong with the statement in Comp" << std::endl;
		return false;
	}

	return true;
}

void display_module(std::ostream &out, const evl_modules &modules) {
	for (evl_modules::const_iterator it1 = modules.begin(); it1 != modules.end(); ++it1) {
		out << "module " << it1->name << " " << it1->c_wires.size() << " " << it1->c_components.size() << std::endl;
		for(evl_wires::const_iterator it2 = it1->c_wires.begin(); it2 != it1->c_wires.end(); ++it2) {
			out << "wire " << it2->first << " " << it2->second << std::endl;
		}
		for (evl_components::const_iterator it3 = it1->c_components.begin(); it3 != it1->c_components.end(); ++it3) {
			out << "component " << it3->type << " " << it3->name << " " << it3->comp_pins.size() << std::endl;
			for(evl_pins::const_iterator it4 = it3->comp_pins.begin(); it4 != it3->comp_pins.end(); ++it4) {
				out << "pin " << it4->pin_name << " " << it4->msb << " " << it4->lsb << std::endl;
			}
		}
	}

}
bool store_module_to_file(std::string file_name, evl_modules &modules) {
	std::ofstream output_file(file_name.c_str());
	if(!output_file) {
		std::cerr << "I can't write Module File." << std::endl;
		return false;
	}
	display_module(output_file, modules);

	std::cout << file_name << std::endl;
	return true;
}

bool parse_evl_file(std::string file_name, evl_modules &modules) {
	evl_tokens tokens;
	if(!extract_tokens_from_file(file_name, tokens)) {
		return false;
	}
	
	//display_tokens(std::cout, tokens);
	if (!store_tokens_to_file(file_name + ".tokens", tokens)) {
		return false;
	}

	evl_statements statements;
	if(!group_tokens_into_statements(statements, tokens)){
		return false;
	}

	//display_statements(std::cout,statements);	//displays before the sorting
	if (!store_statements_to_file(file_name + ".statements", statements)) {
		return false;
	}

	evl_statements::const_iterator mod_itr;
    if (!reorder_statements_list(mod_itr,statements)) {                //Reorders the statements list to put wires first
		return false;
    }
	//display_statements(std::cout,statements);	//displays after the sort is performed
	
	for (evl_statements::iterator iter = statements.begin(); iter != statements.end(); ++iter) {
		if (!process_statements(modules,*iter)) {
			return false;
		}
	}
	
	//display_module(std::cout,modules);
	if (!store_module_to_file(file_name + ".syntax", modules)) {
		return false;
	}
	return true;
}