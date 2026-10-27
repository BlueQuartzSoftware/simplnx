#include "SquareImageFilter.hpp"

#include "ImageProcessing/utils/MathOutputUtilities.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/StreamingStatistics.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

#include <atomic>
#include <type_traits>
#include <utility>

using namespace nx::core;

namespace
{
template <class T>
using SquareOutputType = std::conditional_t<
    std::is_same_v<T, int8>, int16,
    std::conditional_t<
        std::is_same_v<T, uint8>, uint16,
        std::conditional_t<std::is_same_v<T, int16>, int32,
                           std::conditional_t<std::is_same_v<T, uint16>, uint32, std::conditional_t<std::is_same_v<T, int32>, int64, std::conditional_t<std::is_same_v<T, uint32>, uint64, T>>>>>>;

template <class T>
Result<> validateSquareRange(const AbstractDataStore<T>& inputStore, const DataPath& inputPath, const std::atomic_bool& shouldCancel)
{
  static_assert(std::is_same_v<T, int64> || std::is_same_v<T, uint64>);
  if(shouldCancel)
  {
    return {};
  }
  auto extremaResult = ImageProcessing::ComputeArrayMinMax<T>(inputStore, shouldCancel);
  if(extremaResult.invalid())
  {
    return ConvertResult(std::move(extremaResult));
  }
  if(shouldCancel || extremaResult.value().count == 0)
  {
    return ConvertResult(std::move(extremaResult));
  }
  const auto& extrema = extremaResult.value();
  if constexpr(std::is_same_v<T, int64>)
  {
    // Compare native signed values directly; taking abs(INT64_MIN) would overflow.
    constexpr int64 limit = 3037000499LL;
    if(extrema.min < -limit)
    {
      auto errorResult = MakeErrorResult(-1874091, fmt::format("Square Image Filter cannot square value {} in input array '{}': int64 inputs must be at least -3037000499. Convert the input to "
                                                               "float64 first if approximate larger squares are acceptable.",
                                                               extrema.min, inputPath.toString()));
      return MergeResults(ConvertResult(std::move(extremaResult)), std::move(errorResult));
    }
    if(extrema.max > limit)
    {
      auto errorResult = MakeErrorResult(-1874092, fmt::format("Square Image Filter cannot square value {} in input array '{}': int64 inputs must be at most "
                                                               "3037000499. Convert the input to float64 first if approximate larger squares are acceptable.",
                                                               extrema.max, inputPath.toString()));
      return MergeResults(ConvertResult(std::move(extremaResult)), std::move(errorResult));
    }
  }
  else if(extrema.max > 4294967295ULL)
  {
    auto errorResult = MakeErrorResult(-1874093, fmt::format("Square Image Filter cannot square value {} in input array '{}': uint64 inputs must be at most "
                                                             "4294967295. Convert the input to float64 first if approximate larger squares are acceptable.",
                                                             extrema.max, inputPath.toString()));
    return MergeResults(ConvertResult(std::move(extremaResult)), std::move(errorResult));
  }
  return ConvertResult(std::move(extremaResult));
}

struct SquareOperation
{
  template <class T, class U>
  auto makeMapOp() const
  {
    return [](T value) -> U {
      if constexpr(std::is_integral_v<T>)
      {
        const U widenedValue = static_cast<U>(value);
        return static_cast<U>(widenedValue * widenedValue);
      }
      else
      {
        const float64 floatingValue = static_cast<float64>(value);
        return ImageProcessing::ConvertMathResult<U>(floatingValue * floatingValue);
      }
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

  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, SquareOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> SquareImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<> validationResult;
  const DataType inputType = dataStructure.getDataRefAs<IDataArray>(selectedInputArray).getDataType();
  if(inputType == DataType::int64)
  {
    validationResult = validateSquareRange(dataStructure.getDataRefAs<Int64Array>(selectedInputArray).getDataStoreRef(), selectedInputArray, shouldCancel);
  }
  else if(inputType == DataType::uint64)
  {
    validationResult = validateSquareRange(dataStructure.getDataRefAs<UInt64Array>(selectedInputArray).getDataStoreRef(), selectedInputArray, shouldCancel);
  }
  if(validationResult.invalid() || shouldCancel)
  {
    return validationResult;
  }
  const SquareOperation operation{};
  auto executeResult = ImageProcessing::ExecuteImageFilter<ImageProcessing::AllNumeric, SquareOutputType>(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
  return MergeResults(std::move(validationResult), std::move(executeResult));
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
