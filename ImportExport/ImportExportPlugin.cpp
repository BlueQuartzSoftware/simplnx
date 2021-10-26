#include "ImportExportPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{0559aa37-c5ad-549a-82d4-bff4bfcb6cc6}").value(), Uuid::FromString("1c0a7657-c853-5798-ad55-09ec9cca2528").value()},
    {Uuid::FromString("{158ebe9e-f772-57e2-ac1b-71ff213cf890}").value(), Uuid::FromString("714b7fb8-617d-5b80-8843-ee726cb40047").value()},
    {Uuid::FromString("{2861f4b4-8d50-5e69-9575-68c9d35f1256}").value(), Uuid::FromString("4670f91b-dd11-5fb8-8531-fc1ccc1eeef9").value()},
    {Uuid::FromString("{339f1349-9236-5023-9a56-c82fb8eafd12}").value(), Uuid::FromString("1a8be644-8d69-5a9d-8a2c-ed380ce44098").value()},
    {Uuid::FromString("{38f04ea5-d6cd-5baa-8450-ac963570821b}").value(), Uuid::FromString("4e51adbb-cadb-5a35-80c0-5aa291159c3f").value()},
    {Uuid::FromString("{433976f0-710a-5dcc-938e-fcde49cd842f}").value(), Uuid::FromString("663e708d-f701-566f-aa33-7e5ab30bd238").value()},
    {Uuid::FromString("{4786a02e-5fe1-58e0-a906-15556b40d5ce}").value(), Uuid::FromString("efd4ae13-3f91-5cf8-9bce-35f2eca21fd4").value()},
    {Uuid::FromString("{48db4da6-19c3-52a4-894f-776f3a57362e}").value(), Uuid::FromString("ff88b79e-4727-5e6a-9931-1955dcd29318").value()},
    {Uuid::FromString("{85900eba-3da9-5985-ac71-1d9d290a5d31}").value(), Uuid::FromString("5711a174-4c23-52e4-8b2a-119e0b058864").value()},
    {Uuid::FromString("{9072e51c-632f-5ee5-bf6b-9a90bfac2fcf}").value(), Uuid::FromString("51c28415-cdce-5d69-9246-12b719d22992").value()},
    {Uuid::FromString("{980c7bfd-20b2-5711-bc3b-0190b9096c34}").value(), Uuid::FromString("11608280-cec5-55d8-972b-d858a9caa74b").value()},
    {Uuid::FromString("{a043bd66-2681-5126-82e1-5fdc46694bf4}").value(), Uuid::FromString("b3c3570d-ee48-5eb7-89ca-841cde4b3d8e").value()},
    {Uuid::FromString("{abbe2e1e-6fb2-5511-91f3-0637252f0705}").value(), Uuid::FromString("8a8fd732-c561-5e95-aa9a-9211713dad23").value()},
    {Uuid::FromString("{b9134758-d5e5-59dd-9907-28d23e0e0143}").value(), Uuid::FromString("ec2aef6b-2bb4-55de-bb1c-c7c1cea1ab95").value()},
    {Uuid::FromString("{ba2238f8-a20f-5f2f-ac9f-43ed458460f7}").value(), Uuid::FromString("45234e5f-6a43-5300-84dd-3f02965d7b33").value()},
    {Uuid::FromString("{bcf2f246-610f-5575-a434-241d04114b9f}").value(), Uuid::FromString("2ab0a9c4-569c-59a7-9950-f68be3e01605").value()},
    {Uuid::FromString("{c923176f-39c9-5521-9786-624f88d2b2c0}").value(), Uuid::FromString("485b1683-5bf6-5d51-b8d7-5c732c057d05").value()},
    {Uuid::FromString("{f2fecbf9-636b-5ef9-89db-5cb57e4c5676}").value(), Uuid::FromString("ba7fda67-d46c-5f2c-9e00-e6940efd303d").value()},
    {Uuid::FromString("{f62065b4-54e9-53b1-bed7-2178a57d3c7a}").value(), Uuid::FromString("745ead2d-491c-58b2-b376-0d32e16047df").value()},
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("8889d277-c880-5a78-aada-9512585188b9");
} // namespace

ImportExportPlugin::ImportExportPlugin()
: AbstractPlugin(k_ID, "ImportExport", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerFilters();
}

ImportExportPlugin::~ImportExportPlugin() = default;

std::vector<complex::H5::IDataFactory*> ImportExportPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ImportExportPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ImportExport/plugin_filter_registration.h"
