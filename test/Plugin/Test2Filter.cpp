#include "Test2Filter.hpp"

using namespace complex;

Test2Filter::Test2Filter() = default;

Test2Filter::~Test2Filter() = default;

std::string Test2Filter::name() const
{
  return FilterTraits<Test2Filter>::name;
}

complex::Uuid Test2Filter::uuid() const
{
  return FilterTraits<Test2Filter>::uuid;
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
  return std::make_unique<Test2Filter>();
}

Result<OutputActions> Test2Filter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

Result<> Test2Filter::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}
