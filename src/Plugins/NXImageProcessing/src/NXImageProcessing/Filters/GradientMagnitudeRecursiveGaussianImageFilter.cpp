#include "GradientMagnitudeRecursiveGaussianImageFilter.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string GradientMagnitudeRecursiveGaussianImageFilter::name() const
{
  return FilterTraits<GradientMagnitudeRecursiveGaussianImageFilter>::name;
}
std::string GradientMagnitudeRecursiveGaussianImageFilter::className() const
{
  return FilterTraits<GradientMagnitudeRecursiveGaussianImageFilter>::className;
}
Uuid GradientMagnitudeRecursiveGaussianImageFilter::uuid() const
{
  return FilterTraits<GradientMagnitudeRecursiveGaussianImageFilter>::uuid;
}
std::string GradientMagnitudeRecursiveGaussianImageFilter::humanName() const
{
  return "Gradient Magnitude Recursive Gaussian Image Filter";
}
std::vector<std::string> GradientMagnitudeRecursiveGaussianImageFilter::defaultTags() const
{
  return {className(), "NXImageProcessing", "Gradient", "Gaussian", "EdgeDetection"};
}

Parameters GradientMagnitudeRecursiveGaussianImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64ParameterType>(k_Sigma_Key, "Sigma", "The standard deviation of the Gaussian, in the units of image spacing.", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "Normalize Across Scale", "Whether to normalize the Gaussian derivative over scale-space. Default is Off.", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}

IFilter::VersionType GradientMagnitudeRecursiveGaussianImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointerType GradientMagnitudeRecursiveGaussianImageFilter::clone() const
{
  return std::make_unique<GradientMagnitudeRecursiveGaussianImageFilter>();
}

IFilter::PreflightResult GradientMagnitudeRecursiveGaussianImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                                      const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the recursive Gaussian is defined on scalar images. The gradient-magnitude output is a FIXED
  // float32 (matching the legacy ITK FilterOutputType), so AlwaysFloat32Type maps every input type to a float32 output
  // array. AllNumeric admits ALL scalar types INCLUDING floating point (the legacy ScalarPixelIdTypeList).
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::AlwaysFloat32Type>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  if(resultOutputActions.invalid())
  {
    return {.outputActions = std::move(resultOutputActions)};
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  if(Result<> dimCheck = ImageProcessing::ValidateSeparableImageDims(imageGeom.getDimensions(), ImageProcessing::k_RecursiveGaussianTooFewPixels); dimCheck.invalid())
  {
    return {.outputActions = ConvertResultTo<OutputActions>(std::move(dimCheck), OutputActions{})};
  }

  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  if(Result<> sigmaCheck = ImageProcessing::ValidatePositiveSigma(sigma, ImageProcessing::k_RecursiveGaussianNonPositiveSigma); sigmaCheck.invalid())
  {
    return {.outputActions = ConvertResultTo<OutputActions>(std::move(sigmaCheck), OutputActions{})};
  }

  if(Result<> spacingCheck = ImageProcessing::ValidatePositiveSpacing(imageGeom.getSpacing(), imageGeom.getDimensions(), ImageProcessing::k_RecursiveGaussianNonPositiveSpacing, 1e-8);
     spacingCheck.invalid())
  {
    return {.outputActions = ConvertResultTo<OutputActions>(std::move(spacingCheck), OutputActions{})};
  }
  return {.outputActions = std::move(resultOutputActions)};
}

Result<> GradientMagnitudeRecursiveGaussianImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/,
                                                                    const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto sigma = filterArgs.value<float64>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  return ImageProcessing::ExecuteGradientMagnitudeRecursiveGaussianImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, sigma, normalizeAcrossScale, shouldCancel,
                                                                               messageHandler);
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_SigmaKey = "Sigma";
constexpr StringLiteralType k_NormalizeAcrossScaleKey = "NormalizeAcrossScale";
constexpr StringLiteralType k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteralType k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace simpl
} // namespace

Result<Arguments> GradientMagnitudeRecursiveGaussianImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = GradientMagnitudeRecursiveGaussianImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DoubleFilterParameterConverter>(args, json, simpl::k_SigmaKey, k_Sigma_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_NormalizeAcrossScaleKey, k_NormalizeAcrossScale_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
