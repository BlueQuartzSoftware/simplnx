#include "ITKImageProcessingPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{11473711-f94d-5d96-b749-ec36a81ad338}").value(), Uuid::FromString("c9ee33da-3962-5dda-9091-513902d6c3f7").value()},
    {Uuid::FromString("{30687f44-9c10-5617-bcb8-4109cbd6e55e}").value(), Uuid::FromString("605faa74-cddd-57b0-b896-29efff0ae42b").value()},
    {Uuid::FromString("{411b008c-006f-51b2-ba05-99e51a01af3c}").value(), Uuid::FromString("f793164c-d8a4-576a-8cd1-846f8f835a6a").value()},
    {Uuid::FromString("{4388723b-cc16-3477-ac6f-fe0107107e74}").value(), Uuid::FromString("06e9aabd-aaeb-514b-bd53-954aea0befc9").value()},
    {Uuid::FromString("{49b5feb1-ec05-5a26-af25-00053151d944}").value(), Uuid::FromString("13f22db0-d406-5ebd-ab59-0ccc0cfa62ba").value()},
    {Uuid::FromString("{5394f60c-b0f0-5f98-85da-3b5730109060}").value(), Uuid::FromString("ccf8ac91-6e7e-5dcb-b277-5dc0db152583").value()},
    {Uuid::FromString("{53df5340-f632-598f-8a9b-802296b3a95c}").value(), Uuid::FromString("65a00e45-15fe-548a-a761-6e85b6e049f7").value()},
    {Uuid::FromString("{5878723b-cc16-5486-ac5f-ff0107107e74}").value(), Uuid::FromString("edd9c7f3-aed0-51db-b7e8-97571ea409e8").value()},
    {Uuid::FromString("{653b7b5c-03cb-5b32-8c3e-3637745e5ff6}").value(), Uuid::FromString("c434dee5-0ae3-5a0f-86f6-9dba7f8c360f").value()},
    {Uuid::FromString("{774ec947-eed6-5eb2-a01b-ee6470e61521}").value(), Uuid::FromString("59d54885-256e-5860-b37d-70b318e6321b").value()},
    {Uuid::FromString("{7c2a7c4e-4582-52a6-92de-16b626d43fbf}").value(), Uuid::FromString("6b0a7eb8-72fa-57ab-a349-54a29243361a").value()},
    {Uuid::FromString("{a48f7a51-0ca9-584f-a0ca-4bfebdc41d7c}").value(), Uuid::FromString("340ff86c-a203-5ad9-8c87-aa6227ff2009").value()},
    {Uuid::FromString("{c5474cd1-bea9-5a33-a0df-516e5735bab4}").value(), Uuid::FromString("0c8cf5ce-1166-5cbc-8349-c4cd87af7498").value()},
    {Uuid::FromString("{cc27ee9a-9946-56ad-afd4-6e98b71f417d}").value(), Uuid::FromString("9f4e213c-ba3c-5988-9dc1-5554aac5770f").value()},
    {Uuid::FromString("{cdb130af-3616-57b1-be59-fe18113b2621}").value(), Uuid::FromString("1c3f0124-1f39-58a5-bc8c-3439e2fda0ac").value()},
    {Uuid::FromString("{cf7d7497-9573-5102-bedd-38f86a6cdfd4}").value(), Uuid::FromString("63f5addd-cb67-503e-ae22-c8ef0542ec37").value()},
    {Uuid::FromString("{d3856d4c-5651-5eab-8740-489a87fa8bdd}").value(), Uuid::FromString("5d59eecd-7c23-5cdf-9922-d4ab4b907534").value()},
    {Uuid::FromString("{fa4efd40-f4a6-5524-9fc6-e1f8bbb2c42f}").value(), Uuid::FromString("fa4efd40-f4a6-5524-9fc6-e1f8bbb2c42f").value()},
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("115b0d10-ab97-5a18-88e8-80d35056a28e");
} // namespace

ITKImageProcessingPlugin::ITKImageProcessingPlugin()
: AbstractPlugin(k_ID, "ITKImageProcessing", "<<--Description was not read-->>", "BlueQuartz Software")
{
  registerFilters();
}

ITKImageProcessingPlugin::~ITKImageProcessingPlugin() = default;

std::vector<complex::H5::IDataFactory*> ITKImageProcessingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ITKImageProcessingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ITKImageProcessing/plugin_filter_registration.h"
