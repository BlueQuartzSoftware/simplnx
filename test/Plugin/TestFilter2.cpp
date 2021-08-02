#include "Test2Filter.h"

#include <iostream>
#include <string>

std::string Test2Filter::name() const
{
  return "TestFilter2";
}

Uuid Test2Filter::uuid() const
{
  constexpr Uuid uuid = *Uuid::FromString("594343f1-5c84-49d5-8d10-ae781e72f291");
  return uuid;
}

std::string Test2Filter::humanName() const
{
  return "Test Filter 2";
}

complex::Parameters Test2Filter::parameters() const
{
  return {};
}

complex::Result<complex::OutputActions> Test2Filter::preflightImpl(const complex::DataStructure& data, const complex::Arguments& args, const complex::IFilter::MessageHandler& messageHandler) const
{
  std::cout << "Preflight TestFilter2" << std::endl;
  return {};
}

complex::Result<> Test2Filter::executeImpl(complex::DataStructure& data, const complex::Arguments& args, const complex::IFilter::MessageHandler& messageHandler) const
{
  std::cout << "Executing TestFilter2" << std::endl;
  return {};
}