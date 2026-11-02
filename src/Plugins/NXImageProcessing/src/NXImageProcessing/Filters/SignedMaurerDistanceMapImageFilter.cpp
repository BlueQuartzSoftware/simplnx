#include "SignedMaurerDistanceMapImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MaurerDistanceMapEngine.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <algorithm>
#include <optional>

using namespace nx::core;

namespace
{
// BackgroundValue is a Float64 parameter cast to the integer input type at execute; validate its range/finiteness at
// preflight (undefined behavior on a non-finite/out-of-range cast, silent truncation otherwise). Codes are filter-local.
constexpr int32 k_NonFiniteBackgroundValue = -8560;
constexpr int32 k_BackgroundValueOutOfRange = -8561;
constexpr int32 k_BackgroundValueTruncated = -8562;

template <class T>
std::optional<ShapeType> PredictMaurerChunkHint(const SizeVec3& dims)
{
  if(dims[2] <= 1)
  {
    return std::nullopt;
  }
  auto requiredResult = ImageProcessing::detail::CalculateMaurerResidentWorkingMemoryBytes<T>(dims);
  if(requiredResult.invalid())
  {
    return std::nullopt;
  }
  const auto& budgetManager = CacheMemoryBudgetManager::instance();
  const uint64 maximumBytes = budgetManager.maximumWorkingMemoryBytes();
  const uint64 reservedBytes = budgetManager.reservedWorkingMemoryBytes();
  const uint64 availableBytes = maximumBytes > reservedBytes ? maximumBytes - reservedBytes : 0;
  const uint64 preferredBytes = ImageProcessing::ResolvePreferredWorkingMemoryBytes(requiredResult.value());
  const uint64 grantBytes = std::min({static_cast<uint64>(requiredResult.value()), preferredBytes, availableBytes});
  auto hintResult = ImageProcessing::detail::CreateMaurer3DChunkHint<T>(dims, static_cast<usize>(grantBytes));
  return hintResult.valid() ? std::optional<ShapeType>{std::move(hintResult.value())} : std::nullopt;
}

std::optional<ShapeType> PredictMaurerChunkHint(const IDataArray& inputArray, const SizeVec3& dims)
{
  switch(inputArray.getDataType())
  {
  case DataType::int8:
    return PredictMaurerChunkHint<int8>(dims);
  case DataType::uint8:
    return PredictMaurerChunkHint<uint8>(dims);
  case DataType::int16:
    return PredictMaurerChunkHint<int16>(dims);
  case DataType::uint16:
    return PredictMaurerChunkHint<uint16>(dims);
  case DataType::int32:
    return PredictMaurerChunkHint<int32>(dims);
  case DataType::uint32:
    return PredictMaurerChunkHint<uint32>(dims);
  case DataType::int64:
    return PredictMaurerChunkHint<int64>(dims);
  case DataType::uint64:
    return PredictMaurerChunkHint<uint64>(dims);
  default:
    return std::nullopt;
  }
}
} // namespace

namespace nx::core
{
std::string SignedMaurerDistanceMapImageFilter::name() const
{
  return FilterTraits<SignedMaurerDistanceMapImageFilter>::name;
}
std::string SignedMaurerDistanceMapImageFilter::className() const
{
  return FilterTraits<SignedMaurerDistanceMapImageFilter>::className;
}
Uuid SignedMaurerDistanceMapImageFilter::uuid() const
{
  return FilterTraits<SignedMaurerDistanceMapImageFilter>::uuid;
}
std::string SignedMaurerDistanceMapImageFilter::humanName() const
{
  return "Signed Maurer Distance Map Image Filter";
}
std::vector<std::string> SignedMaurerDistanceMapImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "SignedMaurerDistanceMap", "DistanceMap", "MathematicalMorphology", "Morphology"};
}
Parameters SignedMaurerDistanceMapImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_InsideIsPositive_Key, "Inside Is Positive",
                                                "Whether the distance is positive inside the object and negative outside (On) or negative inside and positive outside (Off). Default is Off.", false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "Squared Distance",
                                                "Whether the output stores the squared Euclidean distance (On) or the true Euclidean distance (Off). Default is On.", true));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "Use Image Spacing",
                                                "Whether distances are measured in physical units using the Image Geometry's per-axis spacing (On) or in voxel units (Off). Default is Off.", false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "The input value that marks the background; every voxel whose value is not equal to this is treated as inside the object.", 0.0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetIntegerScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType SignedMaurerDistanceMapImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer SignedMaurerDistanceMapImageFilter::clone() const
{
  return std::make_unique<SignedMaurerDistanceMapImageFilter>();
}
IFilter::PreflightResult SignedMaurerDistanceMapImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the Maurer distance transform is defined on scalar images. The distance output is a FIXED
  // float32 (matching the legacy ITK SignedMaurerDistanceMap FilterOutputType), so AlwaysFloat32 maps every integer
  // input type to a float32 output array. IntegerOnly restricts the input to the integer scalar types.
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::AlwaysFloat32>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, /*requireScalar=*/true, PredictMaurerChunkHint(inputArray, imageGeom.getDimensions()));
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // The Float64 BackgroundValue is cast to the (integer) input element type at execute; a non-finite or out-of-range
  // value is undefined behavior and a fractional value truncates silently, so validate it here. The input is
  // guaranteed present/scalar/integer by the successful preflight above.
  const auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  const DataType inputType = dataStructure.getDataRefAs<IDataArray>(selectedInputArray).getDataType();
  Result<> backgroundGuard = ImageProcessing::ValidateBackgroundInRange(inputType, backgroundValue, k_NonFiniteBackgroundValue, k_BackgroundValueOutOfRange, k_BackgroundValueTruncated);
  if(backgroundGuard.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(backgroundGuard), OutputActions{})};
  }
  for(Warning& warning : backgroundGuard.warnings())
  {
    resultOutputActions.warnings().push_back(std::move(warning));
  }
  return {std::move(resultOutputActions)};
}
Result<> SignedMaurerDistanceMapImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  return ImageProcessing::ExecuteSignedMaurerDistanceMapImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, insideIsPositive, squaredDistance, useImageSpacing,
                                                                    backgroundValue, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InsideIsPositiveKey = "InsideIsPositive";
constexpr StringLiteral k_SquaredDistanceKey = "SquaredDistance";
constexpr StringLiteral k_UseImageSpacingKey = "UseImageSpacing";
constexpr StringLiteral k_BackgroundValueKey = "BackgroundValue";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> SignedMaurerDistanceMapImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = SignedMaurerDistanceMapImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_InsideIsPositiveKey, k_InsideIsPositive_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SquaredDistanceKey, k_SquaredDistance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_UseImageSpacingKey, k_UseImageSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_BackgroundValueKey, k_BackgroundValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
