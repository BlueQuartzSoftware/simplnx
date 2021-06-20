#include "Test2Filter.hpp"

#include <iostream>
#include <string>

#include "Complex/Filtering/AbstractFilter.hpp"

using namespace complex;

const AbstractFilter::IdType Test2Filter::ID = 1;

Test2Filter::Test2Filter()
: AbstractFilter("Test Filter 2", ID)
{
}

Test2Filter::~Test2Filter() = default;

complex::Parameters Test2Filter::parameters() const
{
  return {};
}

bool Test2Filter::preflightImpl(const complex::DataStructure& data, const complex::Arguments& args) const
{
  std::cout << "Preflight TestFilter2" << std::endl;
  return true;
}

void Test2Filter::executeImpl(complex::DataStructure& data, const complex::Arguments& args) const
{
  std::cout << "Executing TestFilter2" << std::endl;
}
