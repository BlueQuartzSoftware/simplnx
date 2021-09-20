#include "TestFilter.hpp"

using namespace complex;

TestFilter::TestFilter() = default;

TestFilter::~TestFilter() = default;

std::string TestFilter::name() const
{
  return FilterTraits<TestFilter>::name.str();
}

complex::Uuid TestFilter::uuid() const
{
  return FilterTraits<TestFilter>::uuid;
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
  return std::make_unique<TestFilter>();
}

complex::Result<OutputActions> TestFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

complex::Result<> TestFilter::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}
