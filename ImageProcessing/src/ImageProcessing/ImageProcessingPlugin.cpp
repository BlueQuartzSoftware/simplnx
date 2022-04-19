#include "ImageProcessingPlugin.hpp"
#include "ImageProcessing/ImageProcessing_filter_registration.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{b1c401e4-cb40-574d-a663-2170b5a7cdaa}").value(), Uuid::FromString("b1c401e4-cb40-574d-a663-2170b5a7cdaa").value()}, /* ItkAutoThreshold */
    {Uuid::FromString("{76fd1b13-5feb-5338-8d7f-b3b935ff3f22}").value(), Uuid::FromString("76fd1b13-5feb-5338-8d7f-b3b935ff3f22").value()}, /* ItkBinaryWatershedLabeled */
    {Uuid::FromString("{52f9e4c4-4e1c-55a3-a316-30c9e99c1216}").value(), Uuid::FromString("52f9e4c4-4e1c-55a3-a316-30c9e99c1216").value()}, /* ItkConvertArrayTo8BitImage */
    {Uuid::FromString("{cd075a60-93a9-52b4-ace6-84342b742c0a}").value(), Uuid::FromString("cd075a60-93a9-52b4-ace6-84342b742c0a").value()}, /* ItkConvertArrayTo8BitImageAttributeMatrix */
    {Uuid::FromString("{73ad993e-a36e-5978-bda2-ccee69d36186}").value(), Uuid::FromString("73ad993e-a36e-5978-bda2-ccee69d36186").value()}, /* ItkDiscreteGaussianBlur */
    {Uuid::FromString("{d2ebf8df-1469-5b77-bfcd-e9e99344749e}").value(), Uuid::FromString("d2ebf8df-1469-5b77-bfcd-e9e99344749e").value()}, /* ItkFindMaxima */
    {Uuid::FromString("{b09afbe3-8483-59ef-b0cd-3dcdaf313f37}").value(), Uuid::FromString("b09afbe3-8483-59ef-b0cd-3dcdaf313f37").value()}, /* ItkGaussianBlur */
    {Uuid::FromString("{d9b598d3-ef06-5776-8f68-362931fa5092}").value(), Uuid::FromString("d9b598d3-ef06-5776-8f68-362931fa5092").value()}, /* ItkGrayToRGB */
    {Uuid::FromString("{98721549-0070-5f80-80d6-0d8d31ade5d7}").value(), Uuid::FromString("98721549-0070-5f80-80d6-0d8d31ade5d7").value()}, /* ItkHoughCircles */
    {Uuid::FromString("{d0916970-0294-5970-aa76-efbee321b56f}").value(), Uuid::FromString("d0916970-0294-5970-aa76-efbee321b56f").value()}, /* ItkImageCalculator */
    {Uuid::FromString("{57b7367b-c4f1-56e0-b47f-e61418479b03}").value(), Uuid::FromString("57b7367b-c4f1-56e0-b47f-e61418479b03").value()}, /* ItkImageMath */
    {Uuid::FromString("{f398f1e8-2e9a-5b5b-b784-f0b8ce7e0abf}").value(), Uuid::FromString("f398f1e8-2e9a-5b5b-b784-f0b8ce7e0abf").value()}, /* ItkKMeans */
    {Uuid::FromString("{68d2b4e5-7325-5c9b-b3e0-26c726e8fd6f}").value(), Uuid::FromString("68d2b4e5-7325-5c9b-b3e0-26c726e8fd6f").value()}, /* ItkKdTreeKMeans */
    {Uuid::FromString("{8a8d3481-34d4-5845-b41f-f89e1d7448b7}").value(), Uuid::FromString("8a8d3481-34d4-5845-b41f-f89e1d7448b7").value()}, /* ItkManualThreshold */
    {Uuid::FromString("{9f6b76ba-cf04-5da1-8e99-783ff481ed85}").value(), Uuid::FromString("9f6b76ba-cf04-5da1-8e99-783ff481ed85").value()}, /* ItkMeanKernel */
    {Uuid::FromString("{303cc321-df55-5bb9-b3a4-22d490b728e7}").value(), Uuid::FromString("303cc321-df55-5bb9-b3a4-22d490b728e7").value()}, /* ItkMedianKernel */
    {Uuid::FromString("{fa281497-2f98-5fc7-bfd7-305dcea866ed}").value(), Uuid::FromString("fa281497-2f98-5fc7-bfd7-305dcea866ed").value()}, /* ItkMultiOtsuThreshold */
    {Uuid::FromString("{73917acf-23aa-59ac-b4fd-ab59a8ce1060}").value(), Uuid::FromString("73917acf-23aa-59ac-b4fd-ab59a8ce1060").value()}, /* ItkSobelEdge */
    {Uuid::FromString("{aed7a137-bf2f-5bbc-b5e6-bf5db18e46c2}").value(), Uuid::FromString("aed7a137-bf2f-5bbc-b5e6-bf5db18e46c2").value()}, /* ItkStitchImages */
    {Uuid::FromString("{994eb3c3-dafa-5611-8afe-efdac8c2f7da}").value(), Uuid::FromString("994eb3c3-dafa-5611-8afe-efdac8c2f7da").value()}, /* ItkWatershed */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("af9f4652-17c1-58f9-9415-1bf6d59fe36c");
} // namespace

ImageProcessingPlugin::ImageProcessingPlugin()
: AbstractPlugin(k_ID, "ImageProcessing", "<<--Description was not read-->>", "Open-Source")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

ImageProcessingPlugin::~ImageProcessingPlugin() = default;

std::vector<complex::H5::IDataFactory*> ImageProcessingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ImageProcessingPlugin)
