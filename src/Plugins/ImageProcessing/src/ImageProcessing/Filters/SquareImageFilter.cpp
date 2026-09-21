#include "SquareImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <cmath>

using namespace nx::core;

namespace
{
struct SquareOperation
{
  template <class T, class U>
  auto makeMapOp() const
  {
    return [](T value) -> U {
      const double d = static_cast<double>(value);
      return static_cast<U>(d * d);
    };
  }
};
} // namespace

namespace nx::core
{
std::string SquareImageFilter::name() const
{
  return FilterTraits<SquareImageFilter>::name;
}
std::string SquareImageFilter::className() const
{
  return FilterTraits<SquareImageFilter>::className;
}
Uuid SquareImageFilter::uuid() const
{
  return FilterTraits<SquareImageFilter>::uuid;
}
std::string SquareImageFilter::humanName() const
{
  return "Square Image Filter";
}
std::vector<std::string> SquareImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Square", "Pointwise"};
}
Parameters SquareImageFilter::parameters() const
{
  Parameters params;
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
IFilter::VersionType SquareImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer SquareImageFilter::clone() const
{
  return std::make_unique<SquareImageFilter>();
}
IFilter::PreflightResult SquareImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                          const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> SquareImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const SquareOperation operation{};
  return ImageProcessing::ExecuteImageFilter(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> SquareImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SquareImageFilter().getDefaultArguments();
  std::vector<Result<>> results;
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));
  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
