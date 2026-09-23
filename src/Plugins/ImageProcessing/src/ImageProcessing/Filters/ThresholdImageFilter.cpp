#include "ThresholdImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
// Output pixel type == input pixel type. Threshold clamps values outside [lower, upper] to
// outsideValue; values within the range pass through unchanged (matches itk::ThresholdImageFilter
// ThresholdOutside with both bounds set). Comparisons are performed in the pixel type T, matching
// ITK's static_cast of the double bounds onto the pixel type.
struct ThresholdOperation
{
  float64 lower = 0.0;
  float64 upper = 1.0;
  float64 outsideValue = 0.0;

  template <class T, class U>
  auto makeMapOp() const
  {
    const T lowerT = static_cast<T>(lower);
    const T upperT = static_cast<T>(upper);
    const U outsideValueU = static_cast<U>(outsideValue);
    return [lowerT, upperT, outsideValueU](T value) -> U { return (value < lowerT || value > upperT) ? outsideValueU : static_cast<U>(value); };
  }
};
} // namespace

namespace nx::core
{
std::string ThresholdImageFilter::name() const
{
  return FilterTraits<ThresholdImageFilter>::name;
}

std::string ThresholdImageFilter::className() const
{
  return FilterTraits<ThresholdImageFilter>::className;
}

Uuid ThresholdImageFilter::uuid() const
{
  return FilterTraits<ThresholdImageFilter>::uuid;
}

std::string ThresholdImageFilter::humanName() const
{
  return "Threshold Image Filter";
}

std::vector<std::string> ThresholdImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Threshold", "Thresholding"};
}

Parameters ThresholdImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Lower_Key, "Lower", "The lower threshold value.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_Upper_Key, "Upper", "The upper threshold value.", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "Outside Value", "Values outside the [Lower, Upper] range are set to this value.", 0.0));

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

IFilter::VersionType ThresholdImageFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer ThresholdImageFilter::clone() const
{
  return std::make_unique<ThresholdImageFilter>();
}

IFilter::PreflightResult ThresholdImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

Result<> ThresholdImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto lower = filterArgs.value<float64>(k_Lower_Key);
  auto upper = filterArgs.value<float64>(k_Upper_Key);
  auto outsideValue = filterArgs.value<float64>(k_OutsideValue_Key);

  const ThresholdOperation operation{lower, upper, outsideValue};

  return ImageProcessing::ExecuteImageFilter(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_LowerKey = "Lower";
constexpr StringLiteral k_UpperKey = "Upper";
constexpr StringLiteral k_OutsideValueKey = "OutsideValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ThresholdImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ThresholdImageFilter().getDefaultArguments();

  std::vector<Result<>> results;
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_LowerKey, k_Lower_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_UpperKey, k_Upper_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutsideValueKey, k_OutsideValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
