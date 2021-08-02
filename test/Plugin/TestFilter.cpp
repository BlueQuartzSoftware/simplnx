#include "TestFilter.hpp"

#include <iostream>
#include <string>

using namespace complex;

const Uuid TestFilter::ID = complex::Uuid::FromString("4a8fb14c-2d42-45c7-86cd-e7d7bb09967f").value();

std::string TestFilter::name() const
{
  return "TestFilter";
}

Uuid TestFilter::uuid() const
{
  return ID;
}

std::string TestFilter::humanName() const
{
  return "Test Filter";
}

Parameters TestFilter::parameters() const
{
  return {};
}

complex::Result<complex::OutputActions> TestFilter::preflightImpl(const complex::DataStructure& data, const complex::Arguments& args, const complex::IFilter::MessageHandler& messageHandler) const
{
  std::cout << "Preflight TestFilter" << std::endl;
  return {};
}

complex::Result<> TestFilter::executeImpl(complex::DataStructure& data, const complex::Arguments& args, const complex::IFilter::MessageHandler& messageHandler) const
{
  std::cout << "Executing TestFilter" << std::endl;
  return {};
}
