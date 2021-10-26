#include "GenericPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{450c2f00-9ddf-56e1-b4c1-0e74e7ad2349}").value(), Uuid::FromString("832cdcfa-a84d-58d9-9b5a-e827f30034d2").value()},
    {Uuid::FromString("{6334ce16-cea5-5643-83b5-9573805873fa}").value(), Uuid::FromString("9df86f97-5d1c-56f6-9cbd-da5daea97f4f").value()},
    {Uuid::FromString("{64d20c7b-697c-5ff1-9d1d-8a27b071f363}").value(), Uuid::FromString("f35c9d2a-6816-528f-a619-14c84327b78d").value()},
    {Uuid::FromString("{6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca}").value(), Uuid::FromString("b9541a03-f5da-52f4-90d5-5b94b5f759b1").value()},
    {Uuid::FromString("{8a1106d4-c67f-5e09-a02a-b2e9b99d031e}").value(), Uuid::FromString("a7a7a0df-8680-57d6-a447-fb41f2c92f1b").value()},
    {Uuid::FromString("{d2b0ae3d-686a-5dc0-a844-66bc0dc8f3cb}").value(), Uuid::FromString("d41f6a5b-f696-5d2d-867a-e6a0d1cff2d5").value()},
    {Uuid::FromString("{ef28de7e-5bdd-57c2-9318-60ba0dfaf7bc}").value(), Uuid::FromString("d4281f41-ba5f-5429-8bbc-257c785505cb").value()},
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("57c4ff38-8348-5336-8f80-79e0dd5cc223");
} // namespace

GenericPlugin::GenericPlugin()
: AbstractPlugin(k_ID, "Generic", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerFilters();
}

GenericPlugin::~GenericPlugin() = default;

std::vector<complex::H5::IDataFactory*> GenericPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(GenericPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Generic/plugin_filter_registration.h"
