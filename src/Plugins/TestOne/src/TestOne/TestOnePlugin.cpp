#include "TestOnePlugin.hpp"

#include "TestOne/Filters/ErrorWarningFilter.hpp"
#include "TestOne/Filters/ExampleFilter1.hpp"
#include "TestOne/Filters/ExampleFilter2.hpp"
#include "TestOne/Filters/TestFilter.hpp"

using namespace complex;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("01ff618b-781f-4ac0-b9ac-43f26ce1854f");
} // namespace

TestOnePlugin::TestOnePlugin()
: AbstractPlugin(k_ID, "TestOne", "Test Plugin", "BlueQuartz Software")
{
  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<TestFilter>(); });
  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<ExampleFilter1>(); });
  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<ExampleFilter2>(); });
  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<ErrorWarningFilter>(); });
}

TestOnePlugin::~TestOnePlugin() = default;

std::vector<complex::H5::IDataFactory*> TestOnePlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(TestOnePlugin)
