#include "SigmoidImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <cmath>

using namespace nx::core;

namespace
{
struct SigmoidOperation
{
  float64 alpha = 1.0;
  float64 beta = 0.0;
  float64 outputMaximum = 255.0;
  float64 outputMinimum = 0.0;

  template <class T, class U>
  auto makeMapOp() const
  {
    const float64 a = alpha, b = beta, oMax = outputMaximum, oMin = outputMinimum;
    return [a, b, oMax, oMin](T value) -> U {
      // Output bounds are Float64 and can exceed a narrow output type; saturate-cast to U to avoid UB.
      const double d = static_cast<double>(value);
      return ImageProcessing::detail::SaturateCastFromDouble<U>((oMax - oMin) * (1.0 / (1.0 + std::exp(-(d - b) / a))) + oMin);
    };
  }
};
} // namespace

namespace nx::core
{
std::string SigmoidImageFilter::name() const
{
  return FilterTraits<SigmoidImageFilter>::name;
}
std::string SigmoidImageFilter::className() const
{
  return FilterTraits<SigmoidImageFilter>::className;
}
Uuid SigmoidImageFilter::uuid() const
{
  return FilterTraits<SigmoidImageFilter>::uuid;
}
std::string SigmoidImageFilter::humanName() const
{
  return "Sigmoid Image Filter";
}
std::vector<std::string> SigmoidImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Sigmoid", "Pointwise"};
}
Parameters SigmoidImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Alpha_Key, "Alpha", "The Alpha value from the Sigmoid equation.", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_Beta_Key, "Beta", "The Beta value from the sigmoid equation", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximum_Key, "Output Maximum", "The maximum output value", 255.0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMinimum_Key, "Output Minimum", "The minimum output value", 0.0));

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
IFilter::VersionType SigmoidImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer SigmoidImageFilter::clone() const
{
  return std::make_unique<SigmoidImageFilter>();
}
IFilter::PreflightResult SigmoidImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                           const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> SigmoidImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto alpha = filterArgs.value<float64>(k_Alpha_Key);
  auto beta = filterArgs.value<float64>(k_Beta_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);

  const SigmoidOperation operation{alpha, beta, outputMaximum, outputMinimum};
  return ImageProcessing::ExecuteImageFilter(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_AlphaKey = "Alpha";
constexpr StringLiteral k_BetaKey = "Beta";
constexpr StringLiteral k_OutputMaximumKey = "OutputMaximum";
constexpr StringLiteral k_OutputMinimumKey = "OutputMinimum";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> SigmoidImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SigmoidImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_AlphaKey, k_Alpha_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BetaKey, k_Beta_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutputMaximumKey, k_OutputMaximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_OutputMinimumKey, k_OutputMinimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
