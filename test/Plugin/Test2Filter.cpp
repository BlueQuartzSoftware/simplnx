#include "Test2Filter.hpp"

#include <iostream>
#include <optional>
#include <string>

using namespace complex;

const complex::Uuid Test2Filter::ID = Uuid::FromString("ad9cf22b-bc5e-41d6-b02e-bb49ffd12c04").value();

Test2Filter::Test2Filter()
: IFilter()
{
}

Test2Filter::~Test2Filter() = default;

std::string Test2Filter::name() const
{
  return "Test2Filter";
}

complex::Uuid Test2Filter::uuid() const
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

complex::IFilter::UniquePointer Test2Filter::clone() const
{
  return UniquePointer(new Test2Filter());
}

Result<OutputActions> Test2Filter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  // std::cout << "Preflight TestFilter2" << std::endl;
  return {};
}

Result<> Test2Filter::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  // std::cout << "Executing TestFilter2" << std::endl;
  return {};
}
