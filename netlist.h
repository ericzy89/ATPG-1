#ifndef GUARD_NETLIST_H
#define GUARD_NETLIST_H

#include <vector>
#include <map>

//forward declarations
class netlist;
class gate;
class net;
class pin;

class pin_value {
	friend class netlist;
public:
	std::vector<std::string> number_of_trans;
	std::vector<std::string> set;
	std::vector<std::string> in;
};


class net {
	friend class netlist;
	net() {logic_value = -1;}
	net(const net &);				//no copy
	net &operator=(const net &);	//no assignment
	std::string net_name;
protected:
	int logic_value;		//-1, 0, 1
	std::list<pin *> connections_;
public:
	void append_pin(pin *p);
	std::string get_net_name();
	bool retrieve_logic_value();
	int get_logic_value();
};

class pin {
	friend class gate;
	pin(const pin &);				//no copy
	pin &operator=(const pin &);	//no assignment
	pin() {};
	bool create(gate *g, size_t pin_index, const evl_pin &p, const std::map<std::string, net *> &netlist_nets);
	
	std::vector<net *> nets_;
protected:
	int pin_direction;		//Input or Output designation
	gate *gate_;
	size_t pin_index_;
public:
	int get_pin_direction();
	void set_as_output();
	void set_as_input();
	net * get_net();
	int get_pin_width();
	void display(std::ostream &out);
	bool get_value();
	std::string get_type();
};

class gate {
	friend class netlist;
	gate(const gate &);				//no copy
	gate &operator=(const gate &);	//no assignment
	std::string type_, name_;
	bool create_pins(const evl_pins &pins, const std::map<std::string, net *> &netlist_nets);
	bool create_pin(const evl_pin &p, size_t pin_index, const std::map<std::string, net *> &netlist_nets);
	virtual bool validate_structural_semantics() = 0;
	virtual void compute_next_state() = 0;
	virtual bool compute_gate() = 0;
protected:
	std::vector<pin*> pins_;
	gate(std::string type, std::string name) {
		type_ = type;
		name_ = name;}
	~gate();
public:
	std::string get_type() const;
	std::string get_name() const;
	void display(std::ostream &out);
	void display_out_gate(std::ostream &out);
	void display_in_gate(std::ostream &out);
	void gate::display_logic_value(std::ostream &out);
	bool compute();
	
};

class netlist {
	
	netlist(const netlist &);				//no copy
	netlist &operator=(const netlist &);	//no assignment
	bool create_nets(const evl_wires &wires);
	void create_net(std::string net_name);
	bool create_gates(const evl_components &comps);
	bool create_gate(const evl_component &c);
	void display_sim_out(std::ostream &out);
	void display_sim_in(std::ostream &out);
	void display_sim_out_results(std::ostream &out);
	void display_sim_in_results(std::ostream &out);
protected:
	std::list<gate *> gates_;
	std::map<std::string, net *> nets_;
public:
	std::string file_name;
	netlist() {};
	~netlist();
	bool create(const evl_wires &wires, const evl_components &components);
	void save(std::string file_name);
	void display_netlist(std::ostream &out);
	void simulate(int cycles);
};

class and_gate: public gate {
	bool state_, next_state_;
	bool validate_structural_semantics();
public:
	void compute_next_state();
	bool compute_gate();
	and_gate(std::string name)
		: gate("and", name), state_(false), next_state_(false) {}
};
class or_gate: public gate {
	
	bool state_, next_state_;
	bool validate_structural_semantics();

public:
	void compute_next_state();
	bool compute_gate();	
	or_gate(std::string name)
		: gate("or", name), state_(false), next_state_(false) {}

};

class xor_gate: public gate {
	
	bool state_, next_state_;
	bool validate_structural_semantics();

public:
	void compute_next_state();
	bool compute_gate();
	xor_gate(std::string name)
		: gate("xor", name), state_(false), next_state_(false) {}

};

class not_gate: public gate {

	bool state_, next_state_;
	bool validate_structural_semantics();
	
public:
	void compute_next_state();
	bool compute_gate();
	not_gate(std::string name)
		: gate("not", name), state_(false), next_state_(false) {}
};

class buffer: public gate {

	bool state_, next_state_;
	bool validate_structural_semantics();
	
public:
	void compute_next_state();
	bool compute_gate();
	buffer(std::string name)
		: gate("buf", name), state_(false), next_state_(false) {}
};

class flip_flop: public gate {

	bool state_, next_state_;
	bool validate_structural_semantics();
	
public:
	void compute_next_state();
	bool compute_gate();
	flip_flop(std::string name)
		: gate("dff", name), state_(false), next_state_(false) {}
};

class one: public gate {

	bool state_, next_state_;
	bool validate_structural_semantics();
	
public:
	void compute_next_state();
	bool compute_gate();
	one(std::string name)
		: gate("one", name), state_(false), next_state_(false) {}
};

class zero: public gate {

	bool state_, next_state_;
	bool validate_structural_semantics();
	
public:
	void compute_next_state();
	bool compute_gate();
	zero(std::string name)
		: gate("zero", name), state_(false), next_state_(false) {}
};

class input: public gate {

	bool state_, next_state_;
	bool validate_structural_semantics();
	
public:
	void compute_next_state();
	bool compute_gate();
	input(std::string name)
		: gate("input", name), state_(false), next_state_(false) {}
};

class output: public gate {

	bool state_, next_state_;
	bool validate_structural_semantics();
	
public:
	void compute_next_state();
	bool compute_gate();
	output(std::string name)
		: gate("output", name), state_(false), next_state_(false) {}
};



#endif