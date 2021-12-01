#include "CorePlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{7ff0ebb3-7b0d-5ff7-b9d8-5147031aca10}").value(), Uuid::FromString("eea49b17-0db2-5bbc-80ef-f44249cc8d55").value()}, /* ArrayCalculator */
    {Uuid::FromString("{a6b50fb0-eb7c-5d9b-9691-825d6a4fe772}").value(), Uuid::FromString("e608eb6c-5f68-5c31-8be9-6389251266fe").value()}, /* CombineAttributeArrays */
    {Uuid::FromString("{334034e9-405f-51a3-9c3c-8d9c955835d9}").value(), Uuid::FromString("58e74e74-24c3-578f-89cc-f1f865f68465").value()}, /* CombineAttributeMatrices */
    {Uuid::FromString("{eb5a89c4-4e71-59b1-9719-d10a652d961e}").value(), Uuid::FromString("ce9e5a27-40ce-5238-8b8c-619b502a891e").value()}, /* ConvertColorToGrayScale */
    {Uuid::FromString("{f4ba5fa4-bb5c-5dd1-9429-0dd86d0ecb37}").value(), Uuid::FromString("93820064-84e9-5ddb-bd95-faab4e0404ec").value()}, /* ConvertData */
    {Uuid::FromString("{99836b75-144b-5126-b261-b411133b5e8a}").value(), Uuid::FromString("2217650b-be8b-5944-8dda-f057a327e953").value()}, /* CopyFeatureArrayToElementArray */
    {Uuid::FromString("{088ef69b-ca98-51a9-97ac-369862015d71}").value(), Uuid::FromString("de360e46-22f4-59c2-8da3-0b1e7203c4df").value()}, /* CopyObject */
    {Uuid::FromString("{93375ef0-7367-5372-addc-baa019b1b341}").value(), Uuid::FromString("55c83dbd-974c-50ba-a543-9ddfa98df229").value()}, /* CreateAttributeMatrix */
    {Uuid::FromString("{77f392fb-c1eb-57da-a1b1-e7acf9239fb8}").value(), Uuid::FromString("ab259022-dfa0-5c1a-a419-7b955f549c39").value()}, /* CreateDataArray */
    {Uuid::FromString("{816fbe6b-7c38-581b-b149-3f839fb65b93}").value(), Uuid::FromString("3c9bdcf0-8781-5acf-8d56-14d5118cbbd5").value()}, /* CreateDataContainer */
    {Uuid::FromString("{94438019-21bb-5b61-a7c3-66974b9a34dc}").value(), Uuid::FromString("a76fe4f4-2d85-5d59-8539-806ee7f6ebdf").value()}, /* CreateFeatureArrayFromElementArray */
    {Uuid::FromString("{9ac220b9-14f9-581a-9bac-5714467589cc}").value(), Uuid::FromString("4b74464d-82ad-5b0a-b4a7-4d824c9c8196").value()}, /* CreateGeometry */
    {Uuid::FromString("{c5bb92df-c1bb-5e57-a2bf-280303a81935}").value(), Uuid::FromString("2fdfc85a-0ab9-581a-b93e-22fcf4a6f453").value()}, /* CreateGridMontage */
    {Uuid::FromString("{e6b9a566-c5eb-5e3a-87de-7fe65d1d12b6}").value(), Uuid::FromString("0df2e789-5bdd-5f60-ba75-2a887bc85585").value()}, /* CreateStringArray */
    {Uuid::FromString("{f28cbf07-f15a-53ca-8c7f-b41a11dae6cc}").value(), Uuid::FromString("55e70987-77e5-5107-a806-37c4f9480378").value()}, /* CropVertexGeometry */
    {Uuid::FromString("{043cbde5-3878-5718-958f-ae75714df0df}").value(), Uuid::FromString("a0eda64c-9604-5b3c-9a43-e810d18dd431").value()}, /* DataContainerReader */
    {Uuid::FromString("{3fcd4c43-9d75-5b86-aad4-4441bc914f37}").value(), Uuid::FromString("d9caded3-2572-5b92-93cb-ac79e7a9cfe3").value()}, /* DataContainerWriter */
    {Uuid::FromString("{8a2308ec-86cd-5636-9a0a-6c7d383e9e7f}").value(), Uuid::FromString("42bdbed3-e78e-55f9-a1a1-3a246fe4f039").value()}, /* ExecuteProcess */
    {Uuid::FromString("{2060a933-b6f5-50fd-9382-a008a5cef17f}").value(), Uuid::FromString("d8680574-fae2-5561-9b08-3d31e2da4fd7").value()}, /* ExtractAttributeArraysFromGeometry */
    {Uuid::FromString("{79d59b85-01e8-5c4a-a6e1-3fd3e2ceffb4}").value(), Uuid::FromString("b852f1a2-186f-58dd-8966-5e3d8b183c98").value()}, /* ExtractComponentAsArray */
    {Uuid::FromString("{bc8a91ca-0cee-59c6-b5cb-acc1aab8617f}").value(), Uuid::FromString("5106c198-e6ad-5746-b8b0-a443b8ef1b0b").value()}, /* ExtractVertexGeometry */
    {Uuid::FromString("{64d1df13-17a2-56a2-90a5-4dfda442b144}").value(), Uuid::FromString("321d3c74-74b1-571f-9851-a66642d79fc8").value()}, /* FeatureCountDecision */
    {Uuid::FromString("{737b8d5a-8622-50f9-9a8a-bfdb57608891}").value(), Uuid::FromString("2646756c-4e32-5212-9348-f6ce8ea0d36b").value()}, /* FeatureDataCSVWriter */
    {Uuid::FromString("{8ec1fc8e-6484-5412-a898-8079986c0a26}").value(), Uuid::FromString("d4688d2a-9e86-5f0b-b534-d02c97a7afbf").value()}, /* FindDerivatives */
    {Uuid::FromString("{0d0a6535-6565-51c5-a3fc-fbc00008606d}").value(), Uuid::FromString("6180ed13-d8f9-574c-8743-82a0461a973e").value()}, /* GenerateColorTable */
    {Uuid::FromString("{829da805-6d7c-5106-8209-aae1c207de15}").value(), Uuid::FromString("fbf48b23-e819-5e5f-a739-6878788a6102").value()}, /* GenerateTiltSeries */
    {Uuid::FromString("{657a79de-7d60-5d9d-9f41-80abb62a3158}").value(), Uuid::FromString("0292e91c-ed6a-53c7-941e-38a7e586b397").value()}, /* GenerateVertexCoordinates */
    {Uuid::FromString("{a7007472-29e5-5d0a-89a6-1aed11b603f8}").value(), Uuid::FromString("3ae39f1f-31ac-58fa-afb2-a5894b67efbf").value()}, /* ImportAsciDataArray */
    {Uuid::FromString("{9e98c3b0-5707-5a3b-b8b5-23ef83b02896}").value(), Uuid::FromString("c9676fa2-68d8-59ee-848e-747353a314d6").value()}, /* ImportHDF5Dataset */
    {Uuid::FromString("{dfab9921-fea3-521c-99ba-48db98e43ff8}").value(), Uuid::FromString("19aacceb-b445-5753-8dcb-369f824f63d7").value()}, /* InitializeData */
    {Uuid::FromString("{0e1c45f6-ed7a-5279-8a5c-a2d5cc6bfead}").value(), Uuid::FromString("50f91ecb-69a9-517c-b104-9c1a77af0ef2").value()}, /* LinkFeatureMapToElementArray */
    {Uuid::FromString("{34a19028-c50b-5dea-af0e-e06c798d3686}").value(), Uuid::FromString("a7205685-4ca2-5b9d-a651-6a7460e49965").value()}, /* MaskCountDecision */
    {Uuid::FromString("{fe2cbe09-8ae1-5bea-9397-fd5741091fdb}").value(), Uuid::FromString("8893fcbd-e3e4-560f-aeda-4021c5f86228").value()}, /* MoveData */
    {Uuid::FromString("{e3702900-a6c1-59e1-9180-b57557a7b193}").value(), Uuid::FromString("0b2737bb-344a-56f9-99c7-bbb7f7f09ea9").value()}, /* MoveMultiData */
    {Uuid::FromString("{014b7300-cf36-5ede-a751-5faf9b119dae}").value(), Uuid::FromString("2106ae76-ec8b-59f7-a932-b07064c7cccd").value()}, /* MultiThresholdObjects */
    {Uuid::FromString("{686d5393-2b02-5c86-b887-dd81a8ae80f2}").value(), Uuid::FromString("9cf4d157-5b1c-5196-be42-d84d1872e7b3").value()}, /* MultiThresholdObjects2 */
    {Uuid::FromString("{8cc2198b-6a9d-5bf4-b8c0-b0878bb57f10}").value(), Uuid::FromString("a3985897-1e81-59f3-bccb-90f4b1b39423").value()}, /* PipelineAnnotation */
    {Uuid::FromString("{0ca83462-8564-54ea-9f4e-e5141974f30b}").value(), Uuid::FromString("395adbcc-0efc-5d65-84fb-1db0eba53236").value()}, /* PostSlackMessage */
    {Uuid::FromString("{0791f556-3d73-5b1e-b275-db3f7bb6850d}").value(), Uuid::FromString("34c88831-d4c4-5d99-a9ba-8adc565f5b74").value()}, /* RawBinaryReader */
    {Uuid::FromString("{bdb978bc-96bf-5498-972c-b509c38b8d50}").value(), Uuid::FromString("d4883870-531f-5065-8de5-aa521b503500").value()}, /* ReadASCIIData */
    {Uuid::FromString("{7b1c8f46-90dd-584a-b3ba-34e16958a7d0}").value(), Uuid::FromString("3904f93a-b609-5f2a-ab4e-f566841866b2").value()}, /* RemoveArrays */
    {Uuid::FromString("{1b4b9941-62e4-52f2-9918-15d48147ab88}").value(), Uuid::FromString("51e14222-cf7f-58b1-bc40-acdd507e00a3").value()}, /* RemoveComponentFromArray */
    {Uuid::FromString("{53a5f731-2858-5e3e-bd43-8f2cf45d90ec}").value(), Uuid::FromString("0e3cb4ea-e40a-5fc1-8a05-f4c81c0d8d2c").value()}, /* RenameAttributeArray */
    {Uuid::FromString("{ee29e6d6-1f59-551b-9350-a696523261d5}").value(), Uuid::FromString("a8c9834e-91fe-505c-a10b-b92cfdb1e7cb").value()}, /* RenameAttributeMatrix */
    {Uuid::FromString("{d53c808f-004d-5fac-b125-0fffc8cc78d6}").value(), Uuid::FromString("595e2dd8-1c90-5b11-b3f9-046d3489d0f5").value()}, /* RenameDataContainer */
    {Uuid::FromString("{a37f2e24-7400-5005-b9a7-b2224570cbe9}").value(), Uuid::FromString("f2455020-b81d-5033-8385-08cbee631ca0").value()}, /* ReplaceValueInArray */
    {Uuid::FromString("{1fe19578-6856-55f2-adc8-2236fac22c25}").value(), Uuid::FromString("eb2d68cd-ee7e-5f64-8f35-60851d388333").value()}, /* RequiredZThickness */
    {Uuid::FromString("{e25d9b4c-2b37-578c-b1de-cf7032b5ef19}").value(), Uuid::FromString("60064e6a-b578-5398-9d8a-019825cfc5ea").value()}, /* RotateSampleRefFrame */
    {Uuid::FromString("{3cf44c27-9149-5548-945a-deef1dc994a8}").value(), Uuid::FromString("390ce8ba-47de-5544-840f-3e70a6c9ebfd").value()}, /* ScaleVolume */
    {Uuid::FromString("{6d3a3852-6251-5d2e-b749-6257fd0d8951}").value(), Uuid::FromString("f279660d-9348-557e-927a-e163bf536f14").value()}, /* SetOriginResolutionImageGeom */
    {Uuid::FromString("{5ecf77f4-a38a-52ab-b4f6-0fb8a9c5cb9c}").value(), Uuid::FromString("1e4a7ac7-fc0a-5627-8e84-f07dc42a1862").value()}, /* SplitAttributeArray */
    {Uuid::FromString("{5fbf9204-2c6c-597b-856a-f4612adbac38}").value(), Uuid::FromString("fc34f494-5096-5279-bfdf-3d8db8774afe").value()}, /* WriteASCIIData */
    {Uuid::FromString("{5e523ec1-49ac-541e-a4ba-6fa725798b91}").value(), Uuid::FromString("bb06c259-ebeb-5d63-b86b-ef5197f6bef5").value()}, /* WriteTriangleGeometry */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("45a0f3fc-8c93-5505-8ac6-182e73313869");
} // namespace

CorePlugin::CorePlugin()
: AbstractPlugin(k_ID, "Core", "<<--Description was not read-->>", "OpenSource")
{
  registerFilters();
}

CorePlugin::~CorePlugin() = default;

std::vector<complex::H5::IDataFactory*> CorePlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(CorePlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Core/plugin_filter_registration.h"
