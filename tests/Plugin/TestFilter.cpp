#include "TestFilter.h"

#include <iostream>
#include <string>

#include "Complex/Filtering/AbstractFilter.h"

using namespace Complex;

TestFilter::TestFilter()
: AbstractFilter("Test Filter")
{
}

TestFilter::~TestFilter() = default;

void TestFilter::preflightImpl(FilterDataOps& data, const Arguments& args)
{
  std::cout << "Preflight TestFilter" << std::endl;
}

void TestFilter::executeImpl(FilterDataOps& data, const Arguments& args)
{
  std::cout << "Executing TestFilter" << std::endl;
}
