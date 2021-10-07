#include "Test2Plugin.hpp"

#include "Test2Filter.hpp"

using namespace complex;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854e");
} // namespace

Test2Plugin::Test2Plugin()
: AbstractPlugin(k_ID, "Test 2 Plugin", "Description", "BlueQuartz Software")
{
  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<Test2Filter>(); });
}

Test2Plugin::~Test2Plugin() = default;

std::vector<complex::H5::IDataFactory*> Test2Plugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(Test2Plugin)
