#ifndef GUARD_CHECKPOINTS_H
#define GUARD_CHECKPOINTS_H

bool make_checkpoints(std::string file_name, evl_modules &modules);
void display_checkpoints(std::ostream &out, const evl_modules &modules);
bool store_checkpoints_to_file(std::string file_name, const evl_modules &modules);

#endif