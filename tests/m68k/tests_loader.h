#ifndef __M68K_TESTS_LOADER_H__
#define __M68K_TESTS_LOADER_H__

#include "test_case.h"

#include <string>
#include <vector>


std::vector<test_case> load_tests(std::string path_to_json_tests);

#endif // __M68K_TESTS_LOADER_H__
