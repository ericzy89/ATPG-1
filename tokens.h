#ifndef GUARD_TOKEN_H
#define GUARD_TOKEN_H

struct evl_token {
	enum token_type {NAME, NUMBER, SINGLE};

	token_type type;
	std::string str;
	int line_no;
};
typedef std::list<evl_token> evl_tokens;

bool issingle(char ch);
bool isname(char ch);
bool extract_tokens_from_line(std::string line, int line_no, evl_tokens &tokens);
bool extract_tokens_from_file(std::string file_name, evl_tokens &tokens);
void display_tokens(std::ostream &out, const evl_tokens &tokens);
bool store_tokens_to_file(std::string file_name, const evl_tokens &tokens);
#endif