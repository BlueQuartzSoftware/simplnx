#include "Test2Filter.hpp"

#include <iostream>
#include <string>

using namespace complex;

const Uuid Test2Filter::ID = complex::Uuid::FromString("6060a977-31f5-442c-be83-aa3fc4f8268c").value();

std::string Test2Filter::name() const
{
  return "Test2Filter";
}

Uuid Test2Filter::uuid() const
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

complex::Result<complex::OutputActions> Test2Filter::preflightImpl(const complex::DataStructure& data, const complex::Arguments& args, const complex::IFilter::MessageHandler& messageHandler) const
{
  std::cout << "Preflight Test2Filter" << std::endl;
  return {};
}

complex::Result<> Test2Filter::executeImpl(complex::DataStructure& data, const complex::Arguments& args, const complex::IFilter::MessageHandler& messageHandler) const
{
  std::cout << "Executing Test2Filter" << std::endl;
  return {};
}