#include <ctype.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <iterator>
#include <sstream>
#include <assert.h>
#include "tokens.h"
#include "statements.h"
#include "netlist.h"
#include <algorithm>
#include <math.h>
#include <iomanip>

std::string make_net_name(std::string wire_name, int i) {
	assert (i >= 0);
	std::ostringstream oss;
	oss << wire_name << "[" << i << "]";
	return oss.str();
}

///////////NET///////////
void net::append_pin(pin *p) {
	connections_.push_back(p);
}

int net::get_logic_value() {
	return logic_value;
}

std::string net::get_net_name() {
	return net_name;
}

bool net::retrieve_logic_value() {
	if(logic_value == -1) {
		for (std::list<pin *>::const_iterator con = connections_.begin(); con != connections_.end(); ++con){
				if((*con)->get_type() != "output" && (*con)->get_pin_direction() == 0){
					logic_value = (*con)->get_value(); 
				}
		}
	}
	if (logic_value == 1)
		return true;
	else
		return false;
}

//////////////////////////

///////////PIN///////////
bool pin::create(gate *g, size_t pin_index, const evl_pin &p, const std::map<std::string, net *> &netlist_nets) {
	
	gate_ = g;
	pin_index_ = pin_index;
	
	std::string net_name = p.pin_name;
	if (p.msb == -1) { //A 1-bit wire
		std::map<std::string, net *>::const_iterator net = netlist_nets.find(net_name);
		nets_.push_back(net->second);
		net->second->append_pin(this);
	}
	else {	//a bus
		for (int i = p.lsb; i <= p.msb; ++i) {
			//search inside the loop
			std::map<std::string, net *>::const_iterator net = netlist_nets.find(make_net_name(net_name,i));
			nets_.push_back(net->second);
			net->second->append_pin(this);
		}
	}
	return true;
}

void pin::set_as_input() {
	pin_direction = 1;
}

void pin::set_as_output() {
	pin_direction = 0;
}

int pin::get_pin_width() {
	return nets_.size();
}

net * pin::get_net() {
	return nets_[0];		
}

bool pin::get_value() {
	if (pin_direction == 1)//pin is input
		return get_net()->retrieve_logic_value();
	else
		return gate_->compute();
	return false;
}

int pin::get_pin_direction(){
	return pin_direction;
}

std::string pin::get_type(){
	return gate_->get_type();
}

//////////////////////////

void pin::display(std::ostream &out) {
	out << "pin " << gate_->get_type() << " " << gate_->get_name() << " " << pin_index_ << std::endl;
}

///////////GATE///////////
bool gate::create_pins(const evl_pins &pins, const std::map<std::string, net*> &netlist_nets) {
	size_t pin_index = 0;
	for (evl_pins::const_iterator p = pins.begin(); p != pins.end(); ++p){
		create_pin(*p,pin_index, netlist_nets);
		++pin_index;
	}
	return validate_structural_semantics();
}
bool gate::create_pin(const evl_pin &p, size_t pin_index, const std::map<std::string, net *> &netlist_nets) {
	pins_.push_back(new pin);
	return pins_.back()->create(this, pin_index, p, netlist_nets);
}
gate::~gate() {
	for (size_t i = 0; i < pins_.size(); ++i) {
		delete pins_[i];
	}
}
std::string gate::get_name() const{
	return name_;
}
std::string gate::get_type() const {
	return type_;
}
void gate::display(std::ostream &out){
	out<< "gate " << get_type() << " " << get_name() << " " << pins_.size() << std::endl;
	for(std::vector<pin*>::size_type i = 0; i != pins_.size(); ++i) {
		out << "pin " << pins_[i]->get_pin_width();
		
		for(std::vector<net *>::size_type j = 0; j != pins_[i]->nets_.size(); ++j) {
			out << " " << pins_[i]->nets_[j]->get_net_name();
		}
		out << std::endl;
	}
}

void gate::display_out_gate(std::ostream &out){
	out << pins_.size() << std::endl;
	for(std::vector<pin*>::size_type i = 0; i != pins_.size(); ++i) {
		out << "pin " << pins_[i]->get_pin_width();
		
		for(std::vector<net *>::size_type j = 0; j != pins_[i]->nets_.size(); ++j) {
			out << " " << pins_[i]->nets_[j]->get_net_name();
		}
		out << std::endl;
	}
}

void gate::display_logic_value(std::ostream &out) {
	
	
	for(std::vector<pin*>::size_type i = 0; i != pins_.size(); ++i) {
		std::string hex;
		int count = 0;

		std::vector<net *>::size_type j = pins_[i]->nets_.size() - 1;
		
		double width_pin ((double)(j+1)/4);
		int hex_width = (int)ceil(width_pin);
		
		for(; j != -1; j--) {
			if(pins_[i]->nets_[j]->get_logic_value() == 1) {
				count = (int)(pow(2.0,(int)j) + count);
			}
		}
		out << std::setfill('0') << std::setw(hex_width) << std::uppercase << std::hex << count << " ";
	}
	out << std::endl;
}

int to_hex(size_t i){
	return 0;
}


void gate::display_in_gate(std::ostream &out){
	out << pins_.size() << " ";
	for(std::vector<pin*>::size_type i = 0; i!= pins_.size(); ++i) {
		out << pins_[i]->get_pin_width() << " ";
	}
}

bool gate::compute() {
	for(size_t i=0; i < pins_.size(); ++i) {
		if(pins_[i]->pin_direction == 1) { //pin is input  
			pins_[i]->get_value();
		}
		else
			return compute_gate();
	}
	return false;	//ADDDDDDDEEEEEEDDDDDDDD TTTTHIIISSS
}
//////////////////////////

/////////NETLIST//////////
netlist::~netlist() {
	for (std::list<gate *>::const_iterator it = gates_.begin(); it != gates_.end(); ++it) {
		delete *it;
	}
	for (std::map<std::string, net *>::const_iterator it2 = nets_.begin(); it2 != nets_.end(); ++it2) {
		delete it2->second;
	}
}
bool netlist::create(const evl_wires &wires, const evl_components &comps) {
	return create_nets(wires) && create_gates(comps);
}
bool netlist::create_nets(const evl_wires &wires) {
	for (evl_wires::const_iterator it = wires.begin(); it != wires.end(); ++it) {
		if (it->second == 1) {
			create_net(it->first);
		}
		else {
			for (int i = 0; i < it->second; ++i) {
				create_net(make_net_name(it->first,i));
			}
		}
	}
	return true;
}
void netlist::create_net(std::string net_name) {
	assert(nets_.find(net_name) == nets_.end());
	nets_[net_name] = new net;
	nets_[net_name]->net_name = net_name;
}
bool netlist::create_gates(const evl_components &comps) {
	for (evl_components::const_iterator it = comps.begin(); it != comps.end(); ++it) {
		create_gate(*it);
	}
	return true;
}
bool netlist::create_gate(const evl_component &c) {
	if (c.type == "and") {
		gates_.push_back(new and_gate(c.name));
	}
	else if (c.type == "or") {
		gates_.push_back(new or_gate(c.name));
	}
	else if (c.type == "xor") {
		gates_.push_back(new xor_gate(c.name));
	}
	else if (c.type == "not") {
		gate *g = new not_gate(c.name);
		gates_.push_back(g);
	}
	else if (c.type == "buf") {
		gate *g = new buffer(c.name);
		gates_.push_back(g);
	}
	else if (c.type == "dff") {
		gate *g = new flip_flop(c.name);
		gates_.push_back(g);
	}
	else if (c.type == "one") {
		gates_.push_back(new one(c.name));
	}
	else if (c.type == "zero") {
		gates_.push_back(new zero(c.name));
	}
	else if (c.type == "input") {
		gates_.push_back(new input(c.name));
	}
	else if (c.type == "output") {
		gates_.push_back(new output(c.name));
	}
	else {
		std::cerr << "Gate does not exist" << c.name << " " << c.type << std::endl;
	}

	return gates_.back()->create_pins(c.comp_pins, nets_);
}
void netlist::save(std::string file_name) {
	std::ofstream output_file(file_name.c_str());
	if(!output_file) {
		std::cerr << "I can't write Netlist File." << std::endl;
	}
	display_netlist(output_file);

	std::cout << file_name << std::endl;
}
void netlist::display_netlist(std::ostream &out) {
	out << nets_.size() << " " << gates_.size() << std::endl;
	for (std::map<std::string, net *>::const_iterator net_it = nets_.begin(); net_it != nets_.end(); ++net_it) {
		out << "net " << net_it->first << " " << net_it->second->connections_.size() << std::endl;
		for (std::list<pin *>::const_iterator pin_iter = net_it->second->connections_.begin();
			pin_iter != net_it->second->connections_.end(); pin_iter++) {
				(*pin_iter)->display(out);
		}
	}
	
	for (std::list<gate *>::const_iterator gate_it = gates_.begin(); gate_it != gates_.end(); ++gate_it) {
		(*gate_it)->display(out);
	}
}

int return_int_value(char value){
	if (value == '0')
		return 0;
	else
		return 1;
}

void netlist::simulate(int cycles) {
	std::ofstream output_file;
   // std::ifstream input_file;
	std::string out_file_name = "";
	std::string in_file_name = "";

	std::vector<int> number_of_trans;
	std::vector<int> set;
	std::vector<std::string> in;
	std::vector<std::string> in_bin_vec;

	for(std::list<gate *>::const_iterator gate_find = gates_.begin(); gate_find != gates_.end(); ++gate_find) {
		if((*gate_find)->get_type() == "output") {
			out_file_name = file_name + "." + (*gate_find)->get_name();
			output_file.open(out_file_name.c_str());
		}
		if((*gate_find)->get_type() == "input") {
			//stores file into 3 vectors, number of trans, set, and in 
			
			in_file_name = netlist::file_name + "." + (*gate_find)->get_name();
			std::ifstream input_file((char *)in_file_name.c_str());
			std::string line;

			for (;getline(input_file, line);){
				std::istringstream ss(line);
				
				int var1, var2;
				std::string var3;

				ss >> var1 >> var2 >> var3;

				number_of_trans.push_back(var1);
				set.push_back(var2);
				in.push_back(var3);

			}
			std::string in_bin;

			for(int i = 1; i != in.size();++i) {
				if(in[i] == "0") {
					in_bin = "0000000000000000";
				}
				else {
					for(int j=0; j != 4;++j) {
						switch (in[i].at(j)) {
							case '0': in_bin.append ("0000"); break;
							case '1': in_bin.append ("0001"); break;
							case '2': in_bin.append ("0010"); break;
							case '3': in_bin.append ("0011"); break;
							case '4': in_bin.append ("0100"); break;
							case '5': in_bin.append ("0101"); break;
							case '6': in_bin.append ("0110"); break;
							case '7': in_bin.append ("0111"); break;
							case '8': in_bin.append ("1000"); break;
							case '9': in_bin.append ("1001"); break;
							case 'a': in_bin.append ("1010"); break;
							case 'b': in_bin.append ("1011"); break;
							case 'c': in_bin.append ("1100"); break;
							case 'd': in_bin.append ("1101"); break;
							case 'e': in_bin.append ("1110"); break;
							case 'f': in_bin.append ("1111"); break;
						}
					}
				}
				in_bin_vec.push_back(in_bin);
				in_bin = "";
			}
			
		}
	}

	//creates file
	display_sim_out(output_file);

	if(!in_bin_vec.empty()) {
		for(int j=1; j != number_of_trans.size(); ++j){		
			for(int i = number_of_trans[j]; i != 0; --i){
				cycles--;
				for(std::map<std::string, net *>::iterator net_it = nets_.begin(); net_it != nets_.end(); ++net_it) {
					net_it->second->logic_value = -1;

					if(net_it->first == "set") {
						net_it->second->logic_value = set[j];
					}
			
					if(net_it->first == "in[0]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(15));
					else if(net_it->first == "in[1]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(14));
					else if(net_it->first == "in[2]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(13));
					else if(net_it->first == "in[3]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(12));
					else if(net_it->first == "in[4]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(11));
					else if(net_it->first == "in[5]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(10));
					else if(net_it->first == "in[6]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(9));
					else if(net_it->first == "in[7]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(8));
					else if(net_it->first == "in[8]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(7));
					else if(net_it->first == "in[9]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(6));
					else if(net_it->first == "in[10]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(5));
					else if(net_it->first == "in[11]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(4));
					else if(net_it->first == "in[12]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(3));
					else if(net_it->first == "in[13]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(2));
					else if(net_it->first == "in[14]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(1));
					else if(net_it->first == "in[15]")
						net_it->second->logic_value = return_int_value(in_bin_vec[j-1].at(0));
				
				}

				for(std::map<std::string, net *>::iterator net_it = nets_.begin(); net_it != nets_.end(); ++net_it) {
					net_it->second->retrieve_logic_value();
				}
		
				for(std::list<gate *>::iterator gate_it = gates_.begin(); gate_it != gates_.end(); ++gate_it) {
					if((*gate_it)->get_type() == "dff") {
						(*gate_it)->compute_next_state();
					}
				}
				display_sim_out_results(output_file);
			}
		}
		for(int i = 0; i != cycles; ++i){

			for(std::map<std::string, net *>::iterator net_it = nets_.begin(); net_it != nets_.end(); ++net_it) {
				net_it->second->logic_value = -1;

				if(net_it->first == "set") {
					net_it->second->logic_value = set.back();
				}
				if(net_it->first == "in[0]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(15));
				else if(net_it->first == "in[1]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(14));
				else if(net_it->first == "in[2]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(13));
				else if(net_it->first == "in[3]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(12));
				else if(net_it->first == "in[4]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(11));
				else if(net_it->first == "in[5]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(10));
				else if(net_it->first == "in[6]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(9));
				else if(net_it->first == "in[7]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(8));
				else if(net_it->first == "in[8]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(7));
				else if(net_it->first == "in[9]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(6));
				else if(net_it->first == "in[10]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(5));
				else if(net_it->first == "in[11]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(4));
				else if(net_it->first == "in[12]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(3));
				else if(net_it->first == "in[13]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(2));
				else if(net_it->first == "in[14]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(1));
				else if(net_it->first == "in[15]")
					net_it->second->logic_value = return_int_value(in_bin_vec.back().at(0));
			}
			for(std::map<std::string, net *>::iterator net_it = nets_.begin(); net_it != nets_.end(); ++net_it) {
				net_it->second->retrieve_logic_value();
			}

			for(std::list<gate *>::iterator gate_it = gates_.begin(); gate_it != gates_.end(); ++gate_it) {
				if((*gate_it)->get_type() == "dff") {
					(*gate_it)->compute_next_state();
				}
			}
			display_sim_out_results(output_file);
		}
	}
	else {
		for(int i = 0; i != cycles; ++i){
			for(std::map<std::string, net *>::iterator net_it = nets_.begin(); net_it != nets_.end(); ++net_it) {
				net_it->second->logic_value = -1;
			}
			for(std::map<std::string, net *>::iterator net_it = nets_.begin(); net_it != nets_.end(); ++net_it) {
				net_it->second->retrieve_logic_value();
			}

			for(std::list<gate *>::iterator gate_it = gates_.begin(); gate_it != gates_.end(); ++gate_it) {
				if((*gate_it)->get_type() == "dff") {
					(*gate_it)->compute_next_state();
				}
			}
			display_sim_out_results(output_file);
		}
	}	
	std::cout << out_file_name << std::endl;
	output_file.close();
}



void netlist::display_sim_out(std::ostream &out) {
	for(std::list<gate *>::const_iterator output_gate = gates_.begin(); output_gate != gates_.end(); ++output_gate) {
		if((*output_gate)->get_type() == "output") {
			(*output_gate)->display_out_gate(out);
		}
	}
}

void netlist::display_sim_out_results(std::ostream &out) {
	for(std::list<gate *>::const_iterator output_gate = gates_.begin(); output_gate != gates_.end(); ++output_gate) {
			if((*output_gate)->get_type() == "output") {
				(*output_gate)->display_logic_value(out);
			}
	}
}

//////////////////////////


///////////GATES//////////


bool and_gate::validate_structural_semantics() {
	if (pins_.size() < 3)
		return false;
	pins_[0]->set_as_output();
	for (size_t i = 1; i < pins_.size(); ++i) {
		pins_[i]->set_as_input();
	}
	return true;
}

//compute output
bool and_gate::compute_gate() {
	//check for input pins
	for(size_t i=0; i < pins_.size(); ++i) {
		if(pins_[i]->get_pin_direction() == 1){
			for (size_t j = 0; j < (size_t)pins_[i]->get_pin_width(); ++j) {
				if(!pins_[i]->get_net()->retrieve_logic_value())
					return false;		
			}
		}
	}
	return true;
}

void and_gate::compute_next_state() {}

bool or_gate::validate_structural_semantics() {
	if (pins_.size() < 3)
		return false;
	pins_[0]->set_as_output();
	for (size_t i = 1; i < pins_.size(); ++i) {
		pins_[i]->set_as_input();
	}
	return true;
}

bool or_gate::compute_gate(){
	//check for input pins
	for(size_t i=0; i < pins_.size(); ++i) {
		if(pins_[i]->get_pin_direction() == 1){
			for (size_t j = 0; j < (size_t)pins_[i]->get_pin_width(); ++j) {
				//if(pins_[i]->get_net()->get_logic_value() == 1)
				if(pins_[i]->get_net()->retrieve_logic_value())	
					return true;		
			}
		}
	}
	return false;
}

void or_gate::compute_next_state() {}

bool xor_gate::validate_structural_semantics() {
	if (pins_.size() < 3)
		return false;
	pins_[0]->set_as_output();
	for (size_t i = 1; i < pins_.size(); ++i) {
		pins_[i]->set_as_input();
	}
	return true;
}
bool xor_gate::compute_gate(){
	//check for input pins
	int num_ones = 0;

	for(size_t i=0; i < pins_.size(); ++i) {
		if(pins_[i]->get_pin_direction() == 1){
			for (size_t j = 0; j < (size_t)pins_[i]->get_pin_width(); ++j) {
				if(pins_[i]->get_net()->retrieve_logic_value())			//change all to retrieve_logic_value
					num_ones++;			
			}
		}
	}

	if(num_ones % 2 != 0)
		return true;
	else
		return false;
}

void xor_gate::compute_next_state() {}

bool not_gate::validate_structural_semantics() {
	if (pins_.size() != 2) return false;
	pins_[0]->set_as_output();
	pins_[1]->set_as_input();
	return true;
}

bool not_gate::compute_gate(){
		//check for input pins
	for(size_t i=0; i < pins_.size(); ++i) {
		if(pins_[i]->get_pin_direction() == 1){
			for (size_t j = 0; j < (size_t)pins_[i]->get_pin_width(); ++j) {
				if(!pins_[i]->get_net()->retrieve_logic_value())
					return true;
				else if (pins_[i]->get_net()->retrieve_logic_value())
					return false;
			}
		}
	}
	return false;		//ADDEDDDD THHHIIISSS
}

void not_gate::compute_next_state() {}

bool buffer::validate_structural_semantics() {
	if (pins_.size() != 2) return false;
	pins_[0]->set_as_output();
	pins_[1]->set_as_input();
	return true;
}

bool buffer::compute_gate(){
	for(size_t i=0; i < pins_.size(); ++i) {
		if(pins_[i]->get_pin_direction() == 1){
			for (size_t j = 0; j < (size_t)pins_[i]->get_pin_width(); ++j) {
				if(!pins_[i]->get_net()->retrieve_logic_value())
					return false;
				else if (pins_[i]->get_net()->retrieve_logic_value())
					return true;
			}
		}
	}
	return false; //ADDDEEEDDD THHHIIISSS
}

void buffer::compute_next_state() {}

bool flip_flop::validate_structural_semantics() {
	if (pins_.size() != 2) return false;
	pins_[0]->set_as_output();
	pins_[1]->set_as_input();
	return true;
}

bool flip_flop::compute_gate(){
	
	return next_state_;
}

void flip_flop::compute_next_state() {
	net *input_net = pins_[1]->get_net();
	next_state_ = input_net->retrieve_logic_value();
}

bool one::validate_structural_semantics() {
	if (pins_.size() < 1)
		return false;
	for (size_t i = 0; i < pins_.size(); ++i) {
		pins_[i]->set_as_output();
	}
	return true;
}

bool one::compute_gate(){
	return true;
}

void one::compute_next_state() {}

bool zero::validate_structural_semantics() {
	if (pins_.size() < 1)
		return false;
	for (size_t i = 0; i < pins_.size(); ++i) {
		pins_[i]->set_as_output();
	}
	return true;
}

bool zero::compute_gate(){
	return false;
}

void zero::compute_next_state() {}

bool input::validate_structural_semantics() {
	if (pins_.size() < 1)
		return false;
	for (size_t i = 0; i < pins_.size(); ++i) {
		pins_[i]->set_as_output();
	}
	return true;
}

bool input::compute_gate(){
	return true;
}

void input::compute_next_state() {}

bool output::validate_structural_semantics() {
	if (pins_.size() < 1)
		return false;
	for (size_t i = 0; i < pins_.size(); ++i) {
		pins_[i]->set_as_input();
	}
	return true;
}

bool output::compute_gate(){
	return true;	
}

void output::compute_next_state() {}




