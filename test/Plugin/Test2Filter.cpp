#include "Test2Filter.h"

#include <iostream>
#include <string>

#include "Complex/Filtering/AbstractFilter.h"

using namespace complex;

Test2Filter::Test2Filter()
: AbstractFilter("Test Filter 2")
{
}

Test2Filter::~Test2Filter() = default;

void Test2Filter::preflightImpl(complex::FilterDataOps& data, const complex::Arguments& args)
{
  std::cout << "Preflight TestFilter2" << std::endl;
}

void Test2Filter::executeImpl(complex::FilterDataOps& data, const complex::Arguments& args)
{
  std::cout << "Executing TestFilter2" << std::endl;
}
