#include "TestFilter.hpp"

#include <iostream>
#include <string>

#include "complex/Filtering/AbstractFilter.hpp"

using namespace complex;

const AbstractFilter::IdType TestFilter::ID = 1;

TestFilter::TestFilter()
: AbstractFilter("Test Filter", TestFilter::ID)
{
}

TestFilter::~TestFilter() = default;

complex::Parameters TestFilter::parameters() const
{
  return {};
}

bool TestFilter::preflightImpl(const DataStructure& data, const Arguments& args) const
{
  std::cout << "Preflight TestFilter" << std::endl;
  return true;
}

void TestFilter::executeImpl(DataStructure& data, const Arguments& args) const
{
  std::cout << "Executing TestFilter" << std::endl;
}
