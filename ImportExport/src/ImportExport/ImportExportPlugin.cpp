#include "ImportExportPlugin.hpp"
#include "ImportExport/ImportExport_filter_registration.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{0559aa37-c5ad-549a-82d4-bff4bfcb6cc6}").value(), Uuid::FromString("0559aa37-c5ad-549a-82d4-bff4bfcb6cc6").value()}, /* AbaqusHexahedronWriter */
    {Uuid::FromString("{abbe2e1e-6fb2-5511-91f3-0637252f0705}").value(), Uuid::FromString("abbe2e1e-6fb2-5511-91f3-0637252f0705").value()}, /* AbaqusSurfaceMeshWriter */
    {Uuid::FromString("{2861f4b4-8d50-5e69-9575-68c9d35f1256}").value(), Uuid::FromString("2861f4b4-8d50-5e69-9575-68c9d35f1256").value()}, /* AvizoRectilinearCoordinateWriter */
    {Uuid::FromString("{339f1349-9236-5023-9a56-c82fb8eafd12}").value(), Uuid::FromString("339f1349-9236-5023-9a56-c82fb8eafd12").value()}, /* AvizoUniformCoordinateWriter */
    {Uuid::FromString("{ba2238f8-a20f-5f2f-ac9f-43ed458460f7}").value(), Uuid::FromString("ba2238f8-a20f-5f2f-ac9f-43ed458460f7").value()}, /* DxReader */
    {Uuid::FromString("{9072e51c-632f-5ee5-bf6b-9a90bfac2fcf}").value(), Uuid::FromString("9072e51c-632f-5ee5-bf6b-9a90bfac2fcf").value()}, /* DxWriter */
    {Uuid::FromString("{38f04ea5-d6cd-5baa-8450-ac963570821b}").value(), Uuid::FromString("38f04ea5-d6cd-5baa-8450-ac963570821b").value()}, /* FeatureInfoReader */
    {Uuid::FromString("{433976f0-710a-5dcc-938e-fcde49cd842f}").value(), Uuid::FromString("433976f0-710a-5dcc-938e-fcde49cd842f").value()}, /* GBCDTriangleDumper */
    {Uuid::FromString("{158ebe9e-f772-57e2-ac1b-71ff213cf890}").value(), Uuid::FromString("158ebe9e-f772-57e2-ac1b-71ff213cf890").value()}, /* LosAlamosFFTWriter */
    {Uuid::FromString("{c923176f-39c9-5521-9786-624f88d2b2c0}").value(), Uuid::FromString("c923176f-39c9-5521-9786-624f88d2b2c0").value()}, /* PhReader */
    {Uuid::FromString("{4786a02e-5fe1-58e0-a906-15556b40d5ce}").value(), Uuid::FromString("4786a02e-5fe1-58e0-a906-15556b40d5ce").value()}, /* PhWriter */
    {Uuid::FromString("{980c7bfd-20b2-5711-bc3b-0190b9096c34}").value(), Uuid::FromString("980c7bfd-20b2-5711-bc3b-0190b9096c34").value()}, /* ReadStlFile */
    {Uuid::FromString("{48db4da6-19c3-52a4-894f-776f3a57362e}").value(), Uuid::FromString("48db4da6-19c3-52a4-894f-776f3a57362e").value()}, /* SPParksDumpReader */
    {Uuid::FromString("{bcf2f246-610f-5575-a434-241d04114b9f}").value(), Uuid::FromString("bcf2f246-610f-5575-a434-241d04114b9f").value()}, /* SPParksSitesWriter */
    {Uuid::FromString("{f62065b4-54e9-53b1-bed7-2178a57d3c7a}").value(), Uuid::FromString("f62065b4-54e9-53b1-bed7-2178a57d3c7a").value()}, /* VisualizeGBCDGMT */
    {Uuid::FromString("{85900eba-3da9-5985-ac71-1d9d290a5d31}").value(), Uuid::FromString("85900eba-3da9-5985-ac71-1d9d290a5d31").value()}, /* VisualizeGBCDPoleFigure */
    {Uuid::FromString("{a043bd66-2681-5126-82e1-5fdc46694bf4}").value(), Uuid::FromString("a043bd66-2681-5126-82e1-5fdc46694bf4").value()}, /* VtkRectilinearGridWriter */
    {Uuid::FromString("{f2fecbf9-636b-5ef9-89db-5cb57e4c5676}").value(), Uuid::FromString("f2fecbf9-636b-5ef9-89db-5cb57e4c5676").value()}, /* VtkStructuredPointsReader */
    {Uuid::FromString("{b9134758-d5e5-59dd-9907-28d23e0e0143}").value(), Uuid::FromString("b9134758-d5e5-59dd-9907-28d23e0e0143").value()}, /* WriteStlFile */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("8889d277-c880-5a78-aada-9512585188b9");
} // namespace

ImportExportPlugin::ImportExportPlugin()
: AbstractPlugin(k_ID, "ImportExport", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

ImportExportPlugin::~ImportExportPlugin() = default;

std::vector<complex::H5::IDataFactory*> ImportExportPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ImportExportPlugin)
