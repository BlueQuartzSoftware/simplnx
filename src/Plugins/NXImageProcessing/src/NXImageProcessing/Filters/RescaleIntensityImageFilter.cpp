#include "RescaleIntensityImageFilter.hpp"

#include <algorithm>

#include "NXImageProcessing/utils/ImageFilterTemplateExecutors.hpp"

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
struct RescaleIntensityOperation
{
  float64 output_minimum = 0.0;
  float64 output_maximum = 255.0;

  template <class T>
  Result<ImageProcessing::ArrayStatistics<T>> computeStatistics(const AbstractDataStore<T>& inputStore, const std::atomic_bool& shouldCancel) const
  {
    Result<ImageProcessing::ArrayMinMax<T>> minMaxResult = ImageProcessing::ComputeArrayMinMax<T>(inputStore, shouldCancel);
    if(minMaxResult.invalid())
    {
      return {nonstd::make_unexpected(std::move(minMaxResult.errors()))};
    }

    ImageProcessing::ArrayStatistics<T> stats;
    stats.count = minMaxResult.value().count;
    stats.min = minMaxResult.value().min;
    stats.max = minMaxResult.value().max;
    return {stats};
  }

  template <class T>
  Result<> validate(const ImageProcessing::ArrayStatistics<T>& /*stats*/) const
  {
    return {};
  }

  template <class T, class U>
  auto makeMapOp(const ImageProcessing::ArrayStatistics<T>& stats) const
  {
    const double inMin = static_cast<double>(stats.min);
    const double inMax = static_cast<double>(stats.max);
    const double outMin = output_minimum;
    const double outMax = output_maximum;
    double scale = 0.0;
    if(inMin != inMax)
    {
      scale = (outMax - outMin) / (inMax - inMin);
    }
    else if(inMax != 0.0)
    {
      scale = (outMax - outMin) / inMax;
    }
    const double shift = outMin - inMin * scale;
    return [scale, shift, outMin, outMax](T value) -> U {
      double v = static_cast<double>(value) * scale + shift;
      // Clamp to the requested output range in the double domain, then saturate-cast to U. The output bounds are
      // Float64 and can exceed a narrow output type (e.g. default OutputMaximum 255 with an int8 image), so
      // casting to U before clamping was undefined behavior; SaturateCastFromDouble clamps to U's range too.
      v = std::min(v, outMax);
      v = std::max(v, outMin);
      return ImageProcessing::detail::SaturateCastFromDouble<U>(v);
    };
  }
};
} // namespace

namespace nx::core
{
std::string RescaleIntensityImageFilter::name() const
{
  return FilterTraits<RescaleIntensityImageFilter>::name;
}
std::string RescaleIntensityImageFilter::className() const
{
  return FilterTraits<RescaleIntensityImageFilter>::className;
}
Uuid RescaleIntensityImageFilter::uuid() const
{
  return FilterTraits<RescaleIntensityImageFilter>::uuid;
}
std::string RescaleIntensityImageFilter::humanName() const
{
  return "Rescale Intensity Image Filter";
}
std::vector<std::string> RescaleIntensityImageFilter::defaultTags() const
{
  return {className(), "NXImageProcessing", "RescaleIntensity", "Pointwise"};
}
Parameters RescaleIntensityImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64ParameterType>(k_OutputMinimum_Key, "Output Minimum", "The minimum output value that is used.", 0.0));
  params.insert(std::make_unique<Float64ParameterType>(k_OutputMaximum_Key, "Output Maximum", "The maximum output value that is used.", 255.0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{}, GetDataArrayOnlyTypes(),
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType RescaleIntensityImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointerType RescaleIntensityImageFilter::clone() const
{
  return std::make_unique<RescaleIntensityImageFilter>();
}
IFilter::PreflightResult RescaleIntensityImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                    const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {.output_actions = std::move(resultOutputActions)};
}
Result<> RescaleIntensityImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);

  const RescaleIntensityOperation operation{.output_minimum = outputMinimum, .output_maximum = outputMaximum};
  return ImageProcessing::ExecuteTwoPassImageFilter(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_OutputMinimumKey = "OutputMinimum";
constexpr StringLiteralType k_OutputMaximumKey = "OutputMaximum";
constexpr StringLiteralType k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteralType k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace simpl
} // namespace

Result<Arguments> RescaleIntensityImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RescaleIntensityImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Output Type parameter is not applicable in NX
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DoubleFilterParameterConverterType>(args, json, simpl::k_OutputMinimumKey, k_OutputMinimum_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DoubleFilterParameterConverterType>(args, json, simpl::k_OutputMaximumKey, k_OutputMaximum_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
