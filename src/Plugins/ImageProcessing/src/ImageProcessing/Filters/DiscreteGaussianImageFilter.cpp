#include "DiscreteGaussianImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string DiscreteGaussianImageFilter::name() const
{
  return FilterTraits<DiscreteGaussianImageFilter>::name;
}
std::string DiscreteGaussianImageFilter::className() const
{
  return FilterTraits<DiscreteGaussianImageFilter>::className;
}
Uuid DiscreteGaussianImageFilter::uuid() const
{
  return FilterTraits<DiscreteGaussianImageFilter>::uuid;
}
std::string DiscreteGaussianImageFilter::humanName() const
{
  return "Discrete Gaussian Image Filter";
}
std::vector<std::string> DiscreteGaussianImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Smoothing", "Gaussian", "Blur"};
}

Parameters DiscreteGaussianImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorFloat64Parameter>(
      k_Variance_Key, "Variance",
      "The variance for the discrete Gaussian kernel. Sets the variance independently for each dimension, but see also SetVariance(const double v) . The default is 0.0 in each dimension. If "
      "UseImageSpacing is true, the units are the physical units of your image. If UseImageSpacing is false then the units are pixels.",
      std::vector<float64>(3, 1.0), std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<UInt32Parameter>(k_MaximumKernelWidth_Key, "MaximumKernelWidth",
                                                  "Set the kernel to be no wider than MaximumKernelWidth pixels, even if MaximumError demands it. The default is 32 pixels.", 32u));
  params.insert(std::make_unique<VectorFloat64Parameter>(
      k_MaximumError_Key, "MaximumError",
      "The algorithm will size the discrete kernel so that the error resulting from truncation of the kernel is no greater than MaximumError. The default is 0.01 in each dimension.",
      std::vector<float64>(3, 0.01), std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(
      std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "Use Image Spacing",
                                      "Set/Get whether or not the filter will use the spacing of the input image in its calculations. Use On to take the image spacing information into account and to "
                                      "specify the Gaussian variance in real world units; use Off to ignore the image spacing and to specify the Gaussian variance in voxel units. Default is On.",
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

IFilter::VersionType DiscreteGaussianImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer DiscreteGaussianImageFilter::clone() const
{
  return std::make_unique<DiscreteGaussianImageFilter>();
}

IFilter::PreflightResult DiscreteGaussianImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the discrete Gaussian is defined on scalar images. The output is SameAsInput (type-preserving),
  // so an input type T produces a T-typed output array. AllNumeric admits ALL scalar types INCLUDING floating point
  // (the legacy ScalarPixelIdTypeList). The FIR convolution handles any dimension size via edge clamping, so this
  // preflight has no minimum-dimension guard following it.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}

Result<> DiscreteGaussianImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto variance = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Variance_Key);
  auto maximumKernelWidth = filterArgs.value<uint32>(k_MaximumKernelWidth_Key);
  auto maximumError = filterArgs.value<VectorFloat64Parameter::ValueType>(k_MaximumError_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  return ImageProcessing::ExecuteDiscreteGaussianImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, variance, maximumKernelWidth, maximumError, useImageSpacing,
                                                             shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_VarianceKey = "Variance";
constexpr StringLiteral k_MaximumKernelWidthKey = "MaximumKernelWidth";
constexpr StringLiteral k_MaximumErrorKey = "MaximumError";
constexpr StringLiteral k_UseImageSpacingKey = "UseImageSpacing";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> DiscreteGaussianImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = DiscreteGaussianImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleVec3FilterParameterConverter>(args, json, SIMPL::k_VarianceKey, k_Variance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint32>>(args, json, SIMPL::k_MaximumKernelWidthKey, k_MaximumKernelWidth_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleVec3FilterParameterConverter>(args, json, SIMPL::k_MaximumErrorKey, k_MaximumError_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UseImageSpacingKey, k_UseImageSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
