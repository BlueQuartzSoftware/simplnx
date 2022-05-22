#include "SIMPLPlugin.hpp"
#include "SIMPL/SIMPL_filter_registration.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{7ff0ebb3-7b0d-5ff7-b9d8-5147031aca10}").value(), Uuid::FromString("7ff0ebb3-7b0d-5ff7-b9d8-5147031aca10").value()}, /* ArrayCalculator */
    {Uuid::FromString("{a6b50fb0-eb7c-5d9b-9691-825d6a4fe772}").value(), Uuid::FromString("a6b50fb0-eb7c-5d9b-9691-825d6a4fe772").value()}, /* CombineAttributeArrays */
    {Uuid::FromString("{334034e9-405f-51a3-9c3c-8d9c955835d9}").value(), Uuid::FromString("334034e9-405f-51a3-9c3c-8d9c955835d9").value()}, /* CombineAttributeMatrices */
    {Uuid::FromString("{eb5a89c4-4e71-59b1-9719-d10a652d961e}").value(), Uuid::FromString("eb5a89c4-4e71-59b1-9719-d10a652d961e").value()}, /* ConvertColorToGrayScale */
    {Uuid::FromString("{f4ba5fa4-bb5c-5dd1-9429-0dd86d0ecb37}").value(), Uuid::FromString("f4ba5fa4-bb5c-5dd1-9429-0dd86d0ecb37").value()}, /* ConvertData */
    {Uuid::FromString("{99836b75-144b-5126-b261-b411133b5e8a}").value(), Uuid::FromString("99836b75-144b-5126-b261-b411133b5e8a").value()}, /* CopyFeatureArrayToElementArray */
    {Uuid::FromString("{088ef69b-ca98-51a9-97ac-369862015d71}").value(), Uuid::FromString("088ef69b-ca98-51a9-97ac-369862015d71").value()}, /* CopyObject */
    {Uuid::FromString("{93375ef0-7367-5372-addc-baa019b1b341}").value(), Uuid::FromString("93375ef0-7367-5372-addc-baa019b1b341").value()}, /* CreateAttributeMatrix */
    {Uuid::FromString("{77f392fb-c1eb-57da-a1b1-e7acf9239fb8}").value(), Uuid::FromString("77f392fb-c1eb-57da-a1b1-e7acf9239fb8").value()}, /* CreateDataArray */
    {Uuid::FromString("{816fbe6b-7c38-581b-b149-3f839fb65b93}").value(), Uuid::FromString("816fbe6b-7c38-581b-b149-3f839fb65b93").value()}, /* CreateDataContainer */
    {Uuid::FromString("{94438019-21bb-5b61-a7c3-66974b9a34dc}").value(), Uuid::FromString("94438019-21bb-5b61-a7c3-66974b9a34dc").value()}, /* CreateFeatureArrayFromElementArray */
    {Uuid::FromString("{9ac220b9-14f9-581a-9bac-5714467589cc}").value(), Uuid::FromString("9ac220b9-14f9-581a-9bac-5714467589cc").value()}, /* CreateGeometry */
    {Uuid::FromString("{c5bb92df-c1bb-5e57-a2bf-280303a81935}").value(), Uuid::FromString("c5bb92df-c1bb-5e57-a2bf-280303a81935").value()}, /* CreateGridMontage */
    {Uuid::FromString("{e6b9a566-c5eb-5e3a-87de-7fe65d1d12b6}").value(), Uuid::FromString("e6b9a566-c5eb-5e3a-87de-7fe65d1d12b6").value()}, /* CreateStringArray */
    {Uuid::FromString("{f28cbf07-f15a-53ca-8c7f-b41a11dae6cc}").value(), Uuid::FromString("f28cbf07-f15a-53ca-8c7f-b41a11dae6cc").value()}, /* CropVertexGeometry */
    {Uuid::FromString("{043cbde5-3878-5718-958f-ae75714df0df}").value(), Uuid::FromString("043cbde5-3878-5718-958f-ae75714df0df").value()}, /* DataContainerReader */
    {Uuid::FromString("{3fcd4c43-9d75-5b86-aad4-4441bc914f37}").value(), Uuid::FromString("3fcd4c43-9d75-5b86-aad4-4441bc914f37").value()}, /* DataContainerWriter */
    {Uuid::FromString("{8a2308ec-86cd-5636-9a0a-6c7d383e9e7f}").value(), Uuid::FromString("8a2308ec-86cd-5636-9a0a-6c7d383e9e7f").value()}, /* ExecuteProcess */
    {Uuid::FromString("{2060a933-b6f5-50fd-9382-a008a5cef17f}").value(), Uuid::FromString("2060a933-b6f5-50fd-9382-a008a5cef17f").value()}, /* ExtractAttributeArraysFromGeometry */
    {Uuid::FromString("{79d59b85-01e8-5c4a-a6e1-3fd3e2ceffb4}").value(), Uuid::FromString("79d59b85-01e8-5c4a-a6e1-3fd3e2ceffb4").value()}, /* ExtractComponentAsArray */
    {Uuid::FromString("{bc8a91ca-0cee-59c6-b5cb-acc1aab8617f}").value(), Uuid::FromString("bc8a91ca-0cee-59c6-b5cb-acc1aab8617f").value()}, /* ExtractVertexGeometry */
    {Uuid::FromString("{64d1df13-17a2-56a2-90a5-4dfda442b144}").value(), Uuid::FromString("64d1df13-17a2-56a2-90a5-4dfda442b144").value()}, /* FeatureCountDecision */
    {Uuid::FromString("{737b8d5a-8622-50f9-9a8a-bfdb57608891}").value(), Uuid::FromString("737b8d5a-8622-50f9-9a8a-bfdb57608891").value()}, /* FeatureDataCSVWriter */
    {Uuid::FromString("{8ec1fc8e-6484-5412-a898-8079986c0a26}").value(), Uuid::FromString("8ec1fc8e-6484-5412-a898-8079986c0a26").value()}, /* FindDerivatives */
    {Uuid::FromString("{0d0a6535-6565-51c5-a3fc-fbc00008606d}").value(), Uuid::FromString("0d0a6535-6565-51c5-a3fc-fbc00008606d").value()}, /* GenerateColorTable */
    {Uuid::FromString("{829da805-6d7c-5106-8209-aae1c207de15}").value(), Uuid::FromString("829da805-6d7c-5106-8209-aae1c207de15").value()}, /* GenerateTiltSeries */
    {Uuid::FromString("{657a79de-7d60-5d9d-9f41-80abb62a3158}").value(), Uuid::FromString("657a79de-7d60-5d9d-9f41-80abb62a3158").value()}, /* GenerateVertexCoordinates */
    {Uuid::FromString("{a7007472-29e5-5d0a-89a6-1aed11b603f8}").value(), Uuid::FromString("a7007472-29e5-5d0a-89a6-1aed11b603f8").value()}, /* ImportAsciDataArray */
    {Uuid::FromString("{9e98c3b0-5707-5a3b-b8b5-23ef83b02896}").value(), Uuid::FromString("9e98c3b0-5707-5a3b-b8b5-23ef83b02896").value()}, /* ImportHDF5Dataset */
    {Uuid::FromString("{dfab9921-fea3-521c-99ba-48db98e43ff8}").value(), Uuid::FromString("dfab9921-fea3-521c-99ba-48db98e43ff8").value()}, /* InitializeData */
    {Uuid::FromString("{0e1c45f6-ed7a-5279-8a5c-a2d5cc6bfead}").value(), Uuid::FromString("0e1c45f6-ed7a-5279-8a5c-a2d5cc6bfead").value()}, /* LinkFeatureMapToElementArray */
    {Uuid::FromString("{34a19028-c50b-5dea-af0e-e06c798d3686}").value(), Uuid::FromString("34a19028-c50b-5dea-af0e-e06c798d3686").value()}, /* MaskCountDecision */
    {Uuid::FromString("{fe2cbe09-8ae1-5bea-9397-fd5741091fdb}").value(), Uuid::FromString("fe2cbe09-8ae1-5bea-9397-fd5741091fdb").value()}, /* MoveData */
    {Uuid::FromString("{e3702900-a6c1-59e1-9180-b57557a7b193}").value(), Uuid::FromString("e3702900-a6c1-59e1-9180-b57557a7b193").value()}, /* MoveMultiData */
    {Uuid::FromString("{014b7300-cf36-5ede-a751-5faf9b119dae}").value(), Uuid::FromString("014b7300-cf36-5ede-a751-5faf9b119dae").value()}, /* MultiThresholdObjects */
    {Uuid::FromString("{686d5393-2b02-5c86-b887-dd81a8ae80f2}").value(), Uuid::FromString("686d5393-2b02-5c86-b887-dd81a8ae80f2").value()}, /* MultiThresholdObjects2 */
    {Uuid::FromString("{8cc2198b-6a9d-5bf4-b8c0-b0878bb57f10}").value(), Uuid::FromString("8cc2198b-6a9d-5bf4-b8c0-b0878bb57f10").value()}, /* PipelineAnnotation */
    {Uuid::FromString("{0ca83462-8564-54ea-9f4e-e5141974f30b}").value(), Uuid::FromString("0ca83462-8564-54ea-9f4e-e5141974f30b").value()}, /* PostSlackMessage */
    {Uuid::FromString("{0791f556-3d73-5b1e-b275-db3f7bb6850d}").value(), Uuid::FromString("0791f556-3d73-5b1e-b275-db3f7bb6850d").value()}, /* RawBinaryReader */
    {Uuid::FromString("{bdb978bc-96bf-5498-972c-b509c38b8d50}").value(), Uuid::FromString("bdb978bc-96bf-5498-972c-b509c38b8d50").value()}, /* ReadASCIIData */
    {Uuid::FromString("{7b1c8f46-90dd-584a-b3ba-34e16958a7d0}").value(), Uuid::FromString("7b1c8f46-90dd-584a-b3ba-34e16958a7d0").value()}, /* RemoveArrays */
    {Uuid::FromString("{1b4b9941-62e4-52f2-9918-15d48147ab88}").value(), Uuid::FromString("1b4b9941-62e4-52f2-9918-15d48147ab88").value()}, /* RemoveComponentFromArray */
    {Uuid::FromString("{53a5f731-2858-5e3e-bd43-8f2cf45d90ec}").value(), Uuid::FromString("53a5f731-2858-5e3e-bd43-8f2cf45d90ec").value()}, /* RenameAttributeArray */
    {Uuid::FromString("{ee29e6d6-1f59-551b-9350-a696523261d5}").value(), Uuid::FromString("ee29e6d6-1f59-551b-9350-a696523261d5").value()}, /* RenameAttributeMatrix */
    {Uuid::FromString("{d53c808f-004d-5fac-b125-0fffc8cc78d6}").value(), Uuid::FromString("d53c808f-004d-5fac-b125-0fffc8cc78d6").value()}, /* RenameDataContainer */
    {Uuid::FromString("{a37f2e24-7400-5005-b9a7-b2224570cbe9}").value(), Uuid::FromString("a37f2e24-7400-5005-b9a7-b2224570cbe9").value()}, /* ReplaceValueInArray */
    {Uuid::FromString("{1fe19578-6856-55f2-adc8-2236fac22c25}").value(), Uuid::FromString("1fe19578-6856-55f2-adc8-2236fac22c25").value()}, /* RequiredZThickness */
    {Uuid::FromString("{e25d9b4c-2b37-578c-b1de-cf7032b5ef19}").value(), Uuid::FromString("e25d9b4c-2b37-578c-b1de-cf7032b5ef19").value()}, /* RotateSampleRefFrame */
    {Uuid::FromString("{3cf44c27-9149-5548-945a-deef1dc994a8}").value(), Uuid::FromString("3cf44c27-9149-5548-945a-deef1dc994a8").value()}, /* ScaleVolume */
    {Uuid::FromString("{6d3a3852-6251-5d2e-b749-6257fd0d8951}").value(), Uuid::FromString("6d3a3852-6251-5d2e-b749-6257fd0d8951").value()}, /* SetImageGeomOriginScalingFilter */
    {Uuid::FromString("{5ecf77f4-a38a-52ab-b4f6-0fb8a9c5cb9c}").value(), Uuid::FromString("5ecf77f4-a38a-52ab-b4f6-0fb8a9c5cb9c").value()}, /* SplitAttributeArray */
    {Uuid::FromString("{5fbf9204-2c6c-597b-856a-f4612adbac38}").value(), Uuid::FromString("5fbf9204-2c6c-597b-856a-f4612adbac38").value()}, /* WriteASCIIData */
    {Uuid::FromString("{5e523ec1-49ac-541e-a4ba-6fa725798b91}").value(), Uuid::FromString("5e523ec1-49ac-541e-a4ba-6fa725798b91").value()}, /* WriteTriangleGeometry */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("45a0f3fc-8c93-5505-8ac6-182e73313869");
} // namespace

SIMPLPlugin::SIMPLPlugin()
: AbstractPlugin(k_ID, "SIMPL", "<<--Description was not read-->>", "OpenSource")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

SIMPLPlugin::~SIMPLPlugin() = default;

std::vector<complex::H5::IDataFactory*> SIMPLPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SIMPLPlugin)
