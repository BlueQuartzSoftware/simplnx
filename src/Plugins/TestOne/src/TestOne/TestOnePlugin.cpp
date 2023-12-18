#include "TestOnePlugin.hpp"
#include "TestOne/TestOne_filter_registration.hpp"

using namespace complex;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("01ff618b-781f-4ac0-b9ac-43f26ce1854f");
} // namespace

TestOnePlugin::TestOnePlugin()
: AbstractPlugin(k_ID, "TestOne", "Test Plugin", "BlueQuartz Software")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

TestOnePlugin::~TestOnePlugin() = default;

AbstractPlugin::SIMPLMapType TestOnePlugin::getSimplToComplexMap() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(TestOnePlugin)
