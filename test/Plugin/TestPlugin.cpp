#include "TestPlugin.hpp"

#include "TestFilter.hpp"

using namespace complex;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
} // namespace

TestPlugin::TestPlugin()
: AbstractPlugin(k_ID, "Test Plugin", "Description", "BlueQuartz Software")
{
  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<TestFilter>(); });
}

TestPlugin::~TestPlugin() = default;

std::vector<complex::IH5DataFactory*> TestPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(TestPlugin)
