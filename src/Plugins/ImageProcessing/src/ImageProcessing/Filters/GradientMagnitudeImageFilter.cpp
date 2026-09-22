#include "GradientMagnitudeImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string GradientMagnitudeImageFilter::name() const
{
  return FilterTraits<GradientMagnitudeImageFilter>::name;
}
std::string GradientMagnitudeImageFilter::className() const
{
  return FilterTraits<GradientMagnitudeImageFilter>::className;
}
Uuid GradientMagnitudeImageFilter::uuid() const
{
  return FilterTraits<GradientMagnitudeImageFilter>::uuid;
}
std::string GradientMagnitudeImageFilter::humanName() const
{
  return "Gradient Magnitude Image Filter";
}
std::vector<std::string> GradientMagnitudeImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Gradient", "EdgeDetection"};
}

Parameters GradientMagnitudeImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "Use Image Spacing",
                                                "Set/Get whether or not the filter will use the spacing of the input image in the computation of the derivatives. Use On to compute the gradient in "
                                                "physical space; use Off to ignore image spacing and to compute the gradient in isotropic voxel space. Default is On.",
                                                true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}

IFilter::VersionType GradientMagnitudeImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer GradientMagnitudeImageFilter::clone() const
{
  return std::make_unique<GradientMagnitudeImageFilter>();
}

IFilter::PreflightResult GradientMagnitudeImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the gradient magnitude is defined on scalar images. The output is a FIXED float32 (matching
  // the legacy ITK FilterOutputType), so AlwaysFloat32 maps every input type to a float32 output array. AllNumeric
  // admits ALL scalar types INCLUDING floating point (the legacy ScalarPixelIdTypeList). Unlike the recursive-Gaussian
  // family, the central-difference gradient magnitude has no minimum-dimension requirement (no dim guard follows).
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat32>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // ITK's GradientMagnitudeImageFilter throws "Image spacing cannot be zero" when UseImageSpacing is on; replicate it
  // as a preflight guard (each axis derivative is scaled by 1/spacing[axis], so a zero spacing divides by zero). Only
  // an exactly-zero spacing is rejected (minSpacing 0.0), matching ITK's check.
  const auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  if(useImageSpacing)
  {
    const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
    if(Result<> spacingCheck = ImageProcessing::ValidatePositiveSpacing(imageGeom.getSpacing(), imageGeom.getDimensions(), ImageProcessing::k_GradientMagnitudeZeroSpacing, 0.0);
       spacingCheck.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(spacingCheck), OutputActions{})};
    }
  }
  return {std::move(resultOutputActions)};
}

Result<> GradientMagnitudeImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  return ImageProcessing::ExecuteGradientMagnitudeImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, useImageSpacing, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_UseImageSpacingKey = "UseImageSpacing";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> GradientMagnitudeImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = GradientMagnitudeImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UseImageSpacingKey, k_UseImageSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
