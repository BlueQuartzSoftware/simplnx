#include "TestFilter.hpp"

#include <iostream>
#include <string>

using namespace complex;

const std::string TestFilter::ID = "1";

TestFilter::TestFilter()
: IFilter()
{
}

TestFilter::~TestFilter() = default;

std::string TestFilter::name() const
{
  return "TestFilter";
}

std::string TestFilter::uuid() const
{
  return ID;
}

std::string TestFilter::humanName() const
{
  return "Test Filter";
}

complex::Parameters TestFilter::parameters() const
{
  return {};
}

IFilter::DataCheckResult TestFilter::dataCheckImpl(const complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler) const
{
  //std::cout << "Preflight TestFilter" << std::endl;
  return {};
}

IFilter::ExecuteResult TestFilter::executeImpl(complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler)
{
  //std::cout << "Executing TestFilter" << std::endl;
  return {};
}
