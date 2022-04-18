#include "UCSBUtilitiesPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{39df0167-1ef8-5e01-8702-c08c8afba1c0}").value(), Uuid::FromString("39df0167-1ef8-5e01-8702-c08c8afba1c0").value()}, /* ClearDataMask */
    {Uuid::FromString("{06d9ebfd-4c69-566a-8f4c-30e133a523e7}").value(), Uuid::FromString("06d9ebfd-4c69-566a-8f4c-30e133a523e7").value()}, /* CopyAttributeArray */
    {Uuid::FromString("{9bc962eb-f363-5caf-9f82-23a26be8ae2f}").value(), Uuid::FromString("9bc962eb-f363-5caf-9f82-23a26be8ae2f").value()}, /* CopyAttributeMatrix */
    {Uuid::FromString("{ac8d51d8-9167-5628-a060-95a8863a76b1}").value(), Uuid::FromString("ac8d51d8-9167-5628-a060-95a8863a76b1").value()}, /* CopyDataContainer */
    {Uuid::FromString("{0cdb2c7f-55cb-5fc7-9108-b0c6826bd803}").value(), Uuid::FromString("0cdb2c7f-55cb-5fc7-9108-b0c6826bd803").value()}, /* FindBoundaryAreas */
    {Uuid::FromString("{ce4e8767-d74e-52a1-b34c-7fe0d1efa3b9}").value(), Uuid::FromString("ce4e8767-d74e-52a1-b34c-7fe0d1efa3b9").value()}, /* FindDirectionalModuli */
    {Uuid::FromString("{a0b4c16f-bfb1-57cf-aba1-eb08b5486abb}").value(), Uuid::FromString("a0b4c16f-bfb1-57cf-aba1-eb08b5486abb").value()}, /* FindModulusMismatch */
    {Uuid::FromString("{7152790d-26a4-571a-8fef-493120eced6d}").value(), Uuid::FromString("7152790d-26a4-571a-8fef-493120eced6d").value()}, /* GenerateMisorientationColors */
    {Uuid::FromString("{436eab43-0531-5e56-9309-5931109a85ca}").value(), Uuid::FromString("436eab43-0531-5e56-9309-5931109a85ca").value()}, /* InputCrystalCompliances */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("2b77f77b-b232-51f1-b983-908dcd51344a");
} // namespace

UCSBUtilitiesPlugin::UCSBUtilitiesPlugin()
: AbstractPlugin(k_ID, "UCSBUtilities", "<<--Description was not read-->>", "University of California, Santa Barbara")
{
  registerPublicFilters();
}

UCSBUtilitiesPlugin::~UCSBUtilitiesPlugin() = default;

std::vector<complex::H5::IDataFactory*> UCSBUtilitiesPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(UCSBUtilitiesPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "UCSBUtilities/UCSBUtilities_filter_registration.hpp"
