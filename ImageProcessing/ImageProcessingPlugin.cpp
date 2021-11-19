#include "ImageProcessingPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{b1c401e4-cb40-574d-a663-2170b5a7cdaa}").value(), Uuid::FromString("b40d24d2-9b75-5fed-ba3c-650211e248f9").value()}, /* ItkAutoThreshold */
    {Uuid::FromString("{76fd1b13-5feb-5338-8d7f-b3b935ff3f22}").value(), Uuid::FromString("c49b84a4-4f3d-52e6-b59e-26e1bce4cd1f").value()}, /* ItkBinaryWatershedLabeled */
    {Uuid::FromString("{52f9e4c4-4e1c-55a3-a316-30c9e99c1216}").value(), Uuid::FromString("4f44b137-b94c-5119-a6d8-d21a3c0fdb54").value()}, /* ItkConvertArrayTo8BitImage */
    {Uuid::FromString("{cd075a60-93a9-52b4-ace6-84342b742c0a}").value(), Uuid::FromString("70b13194-5060-55b0-8127-e969c53fa7a7").value()}, /* ItkConvertArrayTo8BitImageAttributeMatrix */
    {Uuid::FromString("{73ad993e-a36e-5978-bda2-ccee69d36186}").value(), Uuid::FromString("657fffcb-b433-53b1-8197-7196758d8fc9").value()}, /* ItkDiscreteGaussianBlur */
    {Uuid::FromString("{d2ebf8df-1469-5b77-bfcd-e9e99344749e}").value(), Uuid::FromString("9fc72c90-6d5b-53b0-9fd9-577a0a4e8ef2").value()}, /* ItkFindMaxima */
    {Uuid::FromString("{b09afbe3-8483-59ef-b0cd-3dcdaf313f37}").value(), Uuid::FromString("b47eb9d4-0008-520b-b4b6-1602510888e3").value()}, /* ItkGaussianBlur */
    {Uuid::FromString("{d9b598d3-ef06-5776-8f68-362931fa5092}").value(), Uuid::FromString("959ab9f7-3a91-50e5-8106-cd0d9eea0d6c").value()}, /* ItkGrayToRGB */
    {Uuid::FromString("{98721549-0070-5f80-80d6-0d8d31ade5d7}").value(), Uuid::FromString("ad32238c-d377-5c03-8709-9f0078c0f55c").value()}, /* ItkHoughCircles */
    {Uuid::FromString("{d0916970-0294-5970-aa76-efbee321b56f}").value(), Uuid::FromString("ab4ad57f-7f55-533d-ad5f-6d87eb840b3a").value()}, /* ItkImageCalculator */
    {Uuid::FromString("{57b7367b-c4f1-56e0-b47f-e61418479b03}").value(), Uuid::FromString("f43d7f13-af5e-5be5-971a-d2c5920a8b29").value()}, /* ItkImageMath */
    {Uuid::FromString("{f398f1e8-2e9a-5b5b-b784-f0b8ce7e0abf}").value(), Uuid::FromString("f18a87b9-f0db-5e94-a2fa-a979c6ca9c17").value()}, /* ItkKMeans */
    {Uuid::FromString("{68d2b4e5-7325-5c9b-b3e0-26c726e8fd6f}").value(), Uuid::FromString("c6b30893-e0ec-588b-93ed-637c358088d8").value()}, /* ItkKdTreeKMeans */
    {Uuid::FromString("{8a8d3481-34d4-5845-b41f-f89e1d7448b7}").value(), Uuid::FromString("b96467dd-0451-5e99-9680-9fcf69275d9d").value()}, /* ItkManualThreshold */
    {Uuid::FromString("{9f6b76ba-cf04-5da1-8e99-783ff481ed85}").value(), Uuid::FromString("1ee9abd9-1557-5634-a63f-93a246750546").value()}, /* ItkMeanKernel */
    {Uuid::FromString("{303cc321-df55-5bb9-b3a4-22d490b728e7}").value(), Uuid::FromString("eebbb351-2ae6-51c9-a3cb-14f7a5f0a2bf").value()}, /* ItkMedianKernel */
    {Uuid::FromString("{fa281497-2f98-5fc7-bfd7-305dcea866ed}").value(), Uuid::FromString("d985b897-b7f8-5328-a27a-57588b9d2a77").value()}, /* ItkMultiOtsuThreshold */
    {Uuid::FromString("{73917acf-23aa-59ac-b4fd-ab59a8ce1060}").value(), Uuid::FromString("0eb6f352-c133-59b1-a310-b48255d52cd8").value()}, /* ItkSobelEdge */
    {Uuid::FromString("{aed7a137-bf2f-5bbc-b5e6-bf5db18e46c2}").value(), Uuid::FromString("5c5180c6-5d18-593a-a4be-5268e8c5697e").value()}, /* ItkStitchImages */
    {Uuid::FromString("{994eb3c3-dafa-5611-8afe-efdac8c2f7da}").value(), Uuid::FromString("8b18b31b-5b79-53ff-b74a-e2d64670405c").value()}, /* ItkWatershed */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("af9f4652-17c1-58f9-9415-1bf6d59fe36c");
} // namespace

ImageProcessingPlugin::ImageProcessingPlugin()
: AbstractPlugin(k_ID, "ImageProcessing", "<<--Description was not read-->>", "Open-Source")
{
  registerFilters();
}

ImageProcessingPlugin::~ImageProcessingPlugin() = default;

std::vector<complex::H5::IDataFactory*> ImageProcessingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ImageProcessingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ImageProcessing/plugin_filter_registration.h"
