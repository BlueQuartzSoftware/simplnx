#include "VolumeMeshingPlugin.hpp"
#include "VolumeMeshing/VolumeMeshing_filter_registration.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{7fc3ba91-781e-5627-b375-b4d4c402e57f}").value(), Uuid::FromString("7fc3ba91-781e-5627-b375-b4d4c402e57f").value()}, /* CleaveTetVolumeMesh */
    {Uuid::FromString("{4cf2cb82-76ee-504b-8d31-061d52ae6d30}").value(), Uuid::FromString("4cf2cb82-76ee-504b-8d31-061d52ae6d30").value()}, /* FindFeatureSignedDistanceFields */
    {Uuid::FromString("{317afa50-defb-541c-8d42-ea659953bde2}").value(), Uuid::FromString("317afa50-defb-541c-8d42-ea659953bde2").value()}, /* VMFillLevelSetWithTetrahedra */
    {Uuid::FromString("{e2b44fbc-f216-5009-a258-578bbe56b587}").value(), Uuid::FromString("e2b44fbc-f216-5009-a258-578bbe56b587").value()}, /* VMFindDistanceFieldFromTriangleGeometry */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("279232bf-84e0-530c-9882-befdaba3618a");
} // namespace

VolumeMeshingPlugin::VolumeMeshingPlugin()
: AbstractPlugin(k_ID, "VolumeMeshing", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

VolumeMeshingPlugin::~VolumeMeshingPlugin() = default;

std::vector<complex::H5::IDataFactory*> VolumeMeshingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(VolumeMeshingPlugin)
