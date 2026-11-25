#include "ReplaceElementAttributesWithNeighborValuesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_TupleCountMismatchError = -15001;
constexpr int32 k_ComparisonNotInCellDataError = -15003;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReplaceElementAttributesWithNeighborValuesFilter::name() const
{
  return FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReplaceElementAttributesWithNeighborValuesFilter::className() const
{
  return FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReplaceElementAttributesWithNeighborValuesFilter::uuid() const
{
  return FilterTraits<ReplaceElementAttributesWithNeighborValuesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReplaceElementAttributesWithNeighborValuesFilter::humanName() const
{
  return "Replace Element Attributes with Neighbor (Threshold)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReplaceElementAttributesWithNeighborValuesFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Replace Values"};
}

//------------------------------------------------------------------------------
Parameters ReplaceElementAttributesWithNeighborValuesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<Float32ParameterType>(k_MinConfidence_Key, "Threshold Value", "The value to of the threshold", 0.1F));
  params.insert(std::make_unique<ChoicesParameter>(k_SelectedComparison_Key, "Comparison Operator", "The operator to use for comparisons. 0=Less, 1=Greater Than", 0, detail::k_OperationChoices));
  params.insert(std::make_unique<BoolParameter>(
      k_Loop_Key, "Loop Until Gone",
      "Repeat threshold replacement until no cells fail. Returns an error if a complete pass finds no passing neighbor for the remaining cells. Requires a comparison array in the "
      "selected geometry's Cell Attribute Matrix.",
      false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ComparisonDataPath, "Input Comparison Array", "The DataPath to the input array to use for comparison", DataPath{}, GetDataArrayOnlyTypes(),
                                                          nx::core::GetAllDataTypes(), ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReplaceElementAttributesWithNeighborValuesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ReplaceElementAttributesWithNeighborValuesFilter::clone() const
{
  return std::make_unique<ReplaceElementAttributesWithNeighborValuesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReplaceElementAttributesWithNeighborValuesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                                         const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  const auto geometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  const auto inputArrayPath = filterArgs.value<DataPath>(k_ComparisonDataPath);

  if(auto tupleResult = ValidateCellTupleCount(dataStructure, geometryPath, {inputArrayPath}, k_TupleCountMismatchError); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  const auto& imageGeomRef = dataStructure.getDataRefAs<ImageGeom>(geometryPath);
  const auto* cellDataPtr = imageGeomRef.getCellData();
  const bool loopUntilGone = filterArgs.value<bool>(k_Loop_Key);
  // Loop mode must write the same comparison array it reads, including when selected through another parent.
  if(cellDataPtr == nullptr || (loopUntilGone && !cellDataPtr->contains(&dataStructure.getDataRefAs<IDataArray>(inputArrayPath))))
  {
    if(auto cellDataResult = ValidateArrayInGeometryCellData(dataStructure, geometryPath, inputArrayPath, k_ComparisonNotInCellDataError, "Input Comparison Array"); cellDataResult.invalid())
    {
      return {.output_actions = ConvertResultTo<OutputActions>(std::move(cellDataResult), {})};
    }
  }

  nx::core::Result<OutputActions> resultOutputActions;
  const auto cellDataPath = imageGeomRef.getCellDataPath();
  for(const auto& [id, child] : *cellDataPtr)
  {
    if(const auto* arrayPtr = dynamic_cast<const IDataArray*>(child.get()); arrayPtr != nullptr)
    {
      MarkDataPathModified(dataStructure, resultOutputActions, cellDataPath.createChildPath(arrayPtr->getName()));
    }
  }

  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ReplaceElementAttributesWithNeighborValuesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/,
                                                                       const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ReplaceElementAttributesWithNeighborValuesInputValues inputValues;

  inputValues.min_confidence = filterArgs.value<float32>(k_MinConfidence_Key);
  inputValues.selected_comparison = filterArgs.value<ChoicesParameter::ValueType>(k_SelectedComparison_Key);
  inputValues.loop = filterArgs.value<bool>(k_Loop_Key);
  inputValues.input_array_path = filterArgs.value<DataPath>(k_ComparisonDataPath);
  inputValues.selected_image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ReplaceElementAttributesWithNeighborValues(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_MinConfidenceKey = "MinConfidence";
constexpr StringLiteralType k_SelectedComparisonKey = "SelectedComparison";
constexpr StringLiteralType k_LoopKey = "Loop";
constexpr StringLiteralType k_ConfidenceIndexArrayPathKey = "ConfidenceIndexArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> ReplaceElementAttributesWithNeighborValuesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReplaceElementAttributesWithNeighborValuesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<float32>>(args, json, simpl::k_MinConfidenceKey, k_MinConfidence_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_SelectedComparisonKey, k_SelectedComparison_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_LoopKey, k_Loop_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_ConfidenceIndexArrayPathKey, k_ComparisonDataPath));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_ConfidenceIndexArrayPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
