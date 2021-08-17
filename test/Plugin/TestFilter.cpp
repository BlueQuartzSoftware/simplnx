#include "TestFilter.hpp"

#include <iostream>
#include <optional>
#include <string>

using namespace complex;

const complex::Uuid TestFilter::ID = Uuid::FromString("5502c3f7-37a8-4a86-b003-1c856be02491").value();

TestFilter::TestFilter()
: IFilter()
{
}

TestFilter::~TestFilter() = default;

std::string TestFilter::name() const
{
  return "TestFilter";
}

complex::Uuid TestFilter::uuid() const
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

complex::IFilter::UniquePointer TestFilter::clone() const
{
  return UniquePointer(new TestFilter());
}

complex::Result<OutputActions> TestFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

complex::Result<> TestFilter::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}
