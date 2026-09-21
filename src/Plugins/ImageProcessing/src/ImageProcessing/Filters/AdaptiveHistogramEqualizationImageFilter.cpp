#include "AdaptiveHistogramEqualizationImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

#include <array>

using namespace nx::core;

namespace
{
// Error code for this filter. The -85xx blocks up to -8533 are taken (object morphology ends at -8533); this
// filter uses the still-unused -8540 block. Only the non-scalar-input guard is needed (Alpha/Beta are native
// float32 params -> no cast guard; PreflightFullOverwriteImageFilter covers type-membership and geometry/tuple mismatch).
constexpr int32 k_NonScalarInput = -8540;
} // namespace

namespace nx::core
{
std::string AdaptiveHistogramEqualizationImageFilter::name() const
{
  return FilterTraits<AdaptiveHistogramEqualizationImageFilter>::name;
}
std::string AdaptiveHistogramEqualizationImageFilter::className() const
{
  return FilterTraits<AdaptiveHistogramEqualizationImageFilter>::className;
}
Uuid AdaptiveHistogramEqualizationImageFilter::uuid() const
{
  return FilterTraits<AdaptiveHistogramEqualizationImageFilter>::uuid;
}
std::string AdaptiveHistogramEqualizationImageFilter::humanName() const
{
  return "Adaptive Histogram Equalization Image Filter";
}
std::vector<std::string> AdaptiveHistogramEqualizationImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "AdaptiveHistogramEqualization", "ImageStatistics", "HistogramEqualization"};
}
Parameters AdaptiveHistogramEqualizationImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_Radius_Key, "Radius", "Radius of the box window per axis (X, Y, Z). The window is (2*radius+1) per axis.", std::vector<uint32>(3, 5),
                                                          std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(
      std::make_unique<Float32Parameter>(k_Alpha_Key, "Alpha", "Alpha = 0 produces the adaptive histogram equalization (provided beta=0). Alpha = 1 produces an unsharp mask. Default is 0.3.", 0.3f));
  params.insert(std::make_unique<Float32Parameter>(
      k_Beta_Key, "Beta", "If beta = 1 (and alpha = 1) the output image matches the input image. As beta approaches 0, the filter behaves as an unsharp mask. Default is 0.3.", 0.3f));

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
IFilter::VersionType AdaptiveHistogramEqualizationImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer AdaptiveHistogramEqualizationImageFilter::clone() const
{
  return std::make_unique<AdaptiveHistogramEqualizationImageFilter>();
}
IFilter::PreflightResult AdaptiveHistogramEqualizationImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(inputArray.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(k_NonScalarInput, fmt::format("Adaptive Histogram Equalization requires a single-component (scalar) input array, but '{}' has {} components.",
                                                                         selectedInputArray.toString(), inputArray.getNumberOfComponents()))};
  }

  return {ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath)};
}
Result<> AdaptiveHistogramEqualizationImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto radius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_Radius_Key);
  auto alpha = filterArgs.value<float32>(k_Alpha_Key);
  auto beta = filterArgs.value<float32>(k_Beta_Key);

  const std::array<usize, 3> radiusArr = {static_cast<usize>(radius[0]), static_cast<usize>(radius[1]), static_cast<usize>(radius[2])};

  return ImageProcessing::ExecuteAdaptiveHistogramEqualizationImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, radiusArr, alpha, beta, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_RadiusKey = "Radius";
constexpr StringLiteral k_AlphaKey = "Alpha";
constexpr StringLiteral k_BetaKey = "Beta";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> AdaptiveHistogramEqualizationImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AdaptiveHistogramEqualizationImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt32Vec3FilterParameterConverter>(args, json, SIMPL::k_RadiusKey, k_Radius_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_AlphaKey, k_Alpha_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_BetaKey, k_Beta_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
