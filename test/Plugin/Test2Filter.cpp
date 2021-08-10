#include "Test2Filter.hpp"

#include <iostream>
#include <string>

using namespace complex;

const std::string Test2Filter::ID = "1";

Test2Filter::Test2Filter()
: IFilter()
{
}

Test2Filter::~Test2Filter() = default;

std::string Test2Filter::name() const
{
  return "Test2Filter";
}

std::string Test2Filter::uuid() const
{
  return ID;
}

std::string Test2Filter::humanName() const
{
  return "Test Filter 2";
}

complex::Parameters Test2Filter::parameters() const
{
  return {};
}

IFilter::DataCheckResult Test2Filter::dataCheckImpl(const complex::DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  std::cout << "Preflight TestFilter2" << std::endl;
  return {};
}

IFilter::ExecuteResult Test2Filter::executeImpl(complex::DataStructure& data, const Arguments& args, const MessageHandler& messageHandler)
{
  std::cout << "Executing TestFilter2" << std::endl;
  return {};
}
