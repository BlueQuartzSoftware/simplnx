#include "SmoothingRecursiveGaussianImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string SmoothingRecursiveGaussianImageFilter::name() const
{
  return FilterTraits<SmoothingRecursiveGaussianImageFilter>::name;
}
std::string SmoothingRecursiveGaussianImageFilter::className() const
{
  return FilterTraits<SmoothingRecursiveGaussianImageFilter>::className;
}
Uuid SmoothingRecursiveGaussianImageFilter::uuid() const
{
  return FilterTraits<SmoothingRecursiveGaussianImageFilter>::uuid;
}
std::string SmoothingRecursiveGaussianImageFilter::humanName() const
{
  return "Smoothing Recursive Gaussian Image Filter";
}
std::vector<std::string> SmoothingRecursiveGaussianImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Smoothing", "Gaussian", "Blur"};
}

Parameters SmoothingRecursiveGaussianImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Sigma_Key, "Sigma", "The standard deviation of the Gaussian along each axis, in the units of image spacing.",
                                                         std::vector<float64>{1.0, 1.0, 1.0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "Normalize Across Scale",
                                                "Whether to normalize the Gaussian over scale-space (affects derivative magnitude; does not change 0th-order smoothing). Default is Off.", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetSignedScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}

IFilter::VersionType SmoothingRecursiveGaussianImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer SmoothingRecursiveGaussianImageFilter::clone() const
{
  return std::make_unique<SmoothingRecursiveGaussianImageFilter>();
}

IFilter::PreflightResult SmoothingRecursiveGaussianImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the recursive Gaussian is defined on scalar images. The output is SameAsInput (type-preserving),
  // so an input type T produces a float32-free, T-typed output array. The SignedScalar policy admits only signed integer
  // + floating point types (the legacy SignedScalarPixelIdTypeList); unsigned types are rejected.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::SignedScalar, ImageProcessing::SameAsInput>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  if(Result<> dimCheck = ImageProcessing::ValidateSeparableImageDims(imageGeom.getDimensions(), ImageProcessing::k_RecursiveGaussianTooFewPixels); dimCheck.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(dimCheck), OutputActions{})};
  }

  auto sigma = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Sigma_Key);
  if(Result<> sigmaCheck = ImageProcessing::ValidatePositiveSigma(sigma, imageGeom.getDimensions(), ImageProcessing::k_RecursiveGaussianNonPositiveSigma); sigmaCheck.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(sigmaCheck), OutputActions{})};
  }

  if(Result<> spacingCheck = ImageProcessing::ValidatePositiveSpacing(imageGeom.getSpacing(), imageGeom.getDimensions(), ImageProcessing::k_RecursiveGaussianNonPositiveSpacing, 1e-8);
     spacingCheck.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(spacingCheck), OutputActions{})};
  }
  return {std::move(resultOutputActions)};
}

Result<> SmoothingRecursiveGaussianImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto sigma = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Sigma_Key);
  auto normalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  return ImageProcessing::ExecuteSmoothingRecursiveGaussianImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SigmaKey = "Sigma";
constexpr StringLiteral k_NormalizeAcrossScaleKey = "NormalizeAcrossScale";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> SmoothingRecursiveGaussianImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SmoothingRecursiveGaussianImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Legacy ITK Sigma is a per-axis FloatVec3 (SIMPL object {"x","y","z"}). DoubleVec3FilterParameterConverter maps it to
  // the VectorFloat64Parameter ValueType (std::vector<float64>).
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleVec3FilterParameterConverter>(args, json, SIMPL::k_SigmaKey, k_Sigma_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_NormalizeAcrossScaleKey, k_NormalizeAcrossScale_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
