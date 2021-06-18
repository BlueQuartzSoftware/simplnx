#include "Test2Filter.h"

#include <iostream>
#include <string>

#include <Redesign/Filtering/AbstractFilter.h>

Test2Filter::Test2Filter()
: AbstractFilter("Test Filter 2", "unidentified2")
{
}

Test2Filter::~Test2Filter() = default;

bool Test2Filter::preflight_impl()
{
  std::cout << "Preflight TestFilter2" << std::endl;
  return true;
}

void Test2Filter::execute_impl()
{
  std::cout << "Executing TestFilter2" << std::endl;
}
