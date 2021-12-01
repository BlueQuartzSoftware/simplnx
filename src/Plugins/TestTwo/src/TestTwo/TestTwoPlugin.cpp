#include "TestTwoPlugin.hpp"

#include "TestTwo/Filters/Test2Filter.hpp"

using namespace complex;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f33ce1854e");
} // namespace

TestTwoPlugin::TestTwoPlugin()
: AbstractPlugin(k_ID, "TestTwo", "Test Plugin", "BlueQuartz Software")
{
  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<Test2Filter>(); });
}

TestTwoPlugin::~TestTwoPlugin() = default;

std::vector<complex::H5::IDataFactory*> TestTwoPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(TestTwoPlugin)
