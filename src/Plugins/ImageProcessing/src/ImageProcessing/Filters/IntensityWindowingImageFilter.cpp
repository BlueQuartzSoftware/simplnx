#include "IntensityWindowingImageFilter.hpp"

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
struct IntensityWindowingOperation
{
  float64 windowMinimum = 0.0;
  float64 windowMaximum = 255.0;
  float64 outputMinimum = 0.0;
  float64 outputMaximum = 255.0;

  template <class T, class U>
  auto makeMapOp() const
  {
    const float64 wMin = windowMinimum, wMax = windowMaximum, oMin = outputMinimum, oMax = outputMaximum;
    const float64 scale = (oMax - oMin) / (wMax - wMin);
    return [wMin, wMax, oMin, oMax, scale](T value) -> U {
      // Output bounds are Float64 and can exceed a narrow output type; saturate-cast to U to avoid UB.
      const double d = static_cast<double>(value);
      if(d <= wMin)
      {
        return ImageProcessing::detail::SaturateCastFromDouble<U>(oMin);
      }
      if(d >= wMax)
      {
        return ImageProcessing::detail::SaturateCastFromDouble<U>(oMax);
      }
      return ImageProcessing::detail::SaturateCastFromDouble<U>((d - wMin) * scale + oMin);
    };
  }
};
} // namespace

namespace nx::core
{
std::string IntensityWindowingImageFilter::name() const
{
  return FilterTraits<IntensityWindowingImageFilter>::name;
}
std::string IntensityWindowingImageFilter::className() const
{
  return FilterTraits<IntensityWindowingImageFilter>::className;
}
Uuid IntensityWindowingImageFilter::uuid() const
{
  return FilterTraits<IntensityWindowingImageFilter>::uuid;
}
std::string IntensityWindowingImageFilter::humanName() const
{
  return "Intensity Windowing Image Filter";
}
std::vector<std::string> IntensityWindowingImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "IntensityWindowing", "Pointwise"};
}
Parameters IntensityWindowingImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_WindowMinimum_Key, "Window Minimum", "Set/Get the values of the maximum and minimum intensities of the input intensity window.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_WindowMaximum_Key, "Window Maximum", "Set/Get the values of the maximum and minimum intensities of the input intensity window.", 255.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMinimum_Key, "Output Minimum", "Set/Get the values of the maximum and minimum intensities of the output image.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximum_Key, "Output Maximum", "Set/Get the values of the maximum and minimum intensities of the output image.", 255.0));

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
IFilter::VersionType IntensityWindowingImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer IntensityWindowingImageFilter::clone() const
{
  return std::make_unique<IntensityWindowingImageFilter>();
}
IFilter::PreflightResult IntensityWindowingImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> IntensityWindowingImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto windowMinimum = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto windowMaximum = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);

  const IntensityWindowingOperation operation{windowMinimum, windowMaximum, outputMinimum, outputMaximum};
  return ImageProcessing::ExecuteImageFilter(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_WindowMinimumKey = "WindowMinimum";
constexpr StringLiteral k_WindowMaximumKey = "WindowMaximum";
constexpr StringLiteral k_OutputMinimumKey = "OutputMinimum";
constexpr StringLiteral k_OutputMaximumKey = "OutputMaximum";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> IntensityWindowingImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = IntensityWindowingImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_WindowMinimumKey, k_WindowMinimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_WindowMaximumKey, k_WindowMaximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutputMinimumKey, k_OutputMinimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutputMaximumKey, k_OutputMaximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
