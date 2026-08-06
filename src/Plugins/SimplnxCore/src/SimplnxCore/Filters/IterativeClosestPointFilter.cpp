#include "IterativeClosestPointFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/IterativeClosestPoint.hpp"

#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
namespace
{
constexpr int32 k_BadNumIterations = -4502;
} // namespace

//------------------------------------------------------------------------------
std::string IterativeClosestPointFilter::name() const
{
  return FilterTraits<IterativeClosestPointFilter>::name;
}

//------------------------------------------------------------------------------
std::string IterativeClosestPointFilter::className() const
{
  return FilterTraits<IterativeClosestPointFilter>::className;
}

//------------------------------------------------------------------------------
Uuid IterativeClosestPointFilter::uuid() const
{
  return FilterTraits<IterativeClosestPointFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string IterativeClosestPointFilter::humanName() const
{
  return "Iterative Closest Point";
}

//------------------------------------------------------------------------------
std::vector<std::string> IterativeClosestPointFilter::defaultTags() const
{
  return {className(), "Transformation", "Align", "Geometry", "ICP"};
}

//------------------------------------------------------------------------------
Parameters IterativeClosestPointFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt64Parameter>(k_NumIterations_Key, "Number of Iterations", "The number of times to run the algorithm [more increases accuracy]", 1));
  params.insert(std::make_unique<BoolParameter>(k_ApplyTransformation_Key, "Apply Transformation to Moving Geometry", "If checked, geometry will be updated implicitly", false));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_MovingVertexPath_Key, "Moving Vertex Geometry", "The geometry to align [mutable]", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TargetVertexPath_Key, "Target Vertex Geometry", "The geometry to be matched against [immutable]", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformArrayPath_Key, "Output Transform Array", "This is the array to store the transform matrix in", DataPath()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType IterativeClosestPointFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer IterativeClosestPointFilter::clone() const
{
  return std::make_unique<IterativeClosestPointFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult IterativeClosestPointFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto movingVertexPath = filterArgs.value<DataPath>(k_MovingVertexPath_Key);
  auto numIterations = filterArgs.value<uint64>(k_NumIterations_Key);
  auto transformArrayPath = filterArgs.value<DataPath>(k_TransformArrayPath_Key);

  if(numIterations < 1)
  {
    auto ss = fmt::format("Must perform at least 1 iteration. Current value: {}", numIterations);
    return {MakeErrorResult<OutputActions>(k_BadNumIterations, ss)};
  }

  auto action = std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{4, 4}, std::vector<usize>{1}, transformArrayPath);

  nx::core::Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::move(action));

  // If the transformation will be applied, the moving geometry's vertices are modified in place
  auto applyTransformation = filterArgs.value<bool>(k_ApplyTransformation_Key);
  if(applyTransformation)
  {
    nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, movingVertexPath, {});
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> IterativeClosestPointFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  IterativeClosestPointInputValues inputValues;

  inputValues.ApplyTransformation = filterArgs.value<bool>(k_ApplyTransformation_Key);
  inputValues.NumIterations = filterArgs.value<uint64>(k_NumIterations_Key);
  inputValues.MovingVertexPath = filterArgs.value<DataPath>(k_MovingVertexPath_Key);
  inputValues.TargetVertexPath = filterArgs.value<DataPath>(k_TargetVertexPath_Key);
  inputValues.TransformArrayPath = filterArgs.value<DataPath>(k_TransformArrayPath_Key);

  return IterativeClosestPoint(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MovingVertexGeometryKey = "MovingVertexGeometry";
constexpr StringLiteral k_TargetVertexGeometryKey = "TargetVertexGeometry";
constexpr StringLiteral k_IterationsKey = "Iterations";
constexpr StringLiteral k_ApplyTransformKey = "ApplyTransform";
constexpr StringLiteral k_TransformArrayNameKey = "TransformArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> IterativeClosestPointFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = IterativeClosestPointFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_MovingVertexGeometryKey, k_MovingVertexPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_TargetVertexGeometryKey, k_TargetVertexPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_IterationsKey, k_NumIterations_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_ApplyTransformKey, k_ApplyTransformation_Key));
  // Transform attribute matrix parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_TransformArrayNameKey, k_TransformArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
