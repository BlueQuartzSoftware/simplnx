#include "TestTwoPlugin.hpp"
#include "TestTwo/TestTwo_filter_registration.hpp"

using namespace complex;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f33ce1854e");
} // namespace

TestTwoPlugin::TestTwoPlugin()
: AbstractPlugin(k_ID, "TestTwo", "Test Plugin", "BlueQuartz Software")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

TestTwoPlugin::~TestTwoPlugin() = default;

AbstractPlugin::SIMPLMapType TestTwoPlugin::getSimplToComplexMap() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(TestTwoPlugin)
