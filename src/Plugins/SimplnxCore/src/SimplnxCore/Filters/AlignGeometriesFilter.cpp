#include "AlignGeometriesFilter.hpp"

#include "Algorithms/AlignGeometries.hpp"

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <string>

using namespace nx::core;

namespace nx::core
{

//------------------------------------------------------------------------------
std::string AlignGeometriesFilter::name() const
{
  return FilterTraits<AlignGeometriesFilter>::name;
}

//------------------------------------------------------------------------------
std::string AlignGeometriesFilter::className() const
{
  return FilterTraits<AlignGeometriesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AlignGeometriesFilter::uuid() const
{
  return FilterTraits<AlignGeometriesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignGeometriesFilter::humanName() const
{
  return "Align Geometries";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignGeometriesFilter::defaultTags() const
{
  return {className(), "Match", "Align", "Geometry", "Move"};
}

//------------------------------------------------------------------------------
Parameters AlignGeometriesFilter::parameters() const
{
  GeometrySelectionParameter::AllowedTypes geomTypes = IGeometry::GetAllGeomTypes();

  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_MovingGeometry_Key, "Moving Geometry", "The geometry that will be moved.", DataPath(), geomTypes));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TargetGeometry_Key, "Fixed Geometry", "The geometry that does *not* move.", DataPath(), geomTypes));
  params.insert(std::make_unique<ChoicesParameter>(k_AlignmentType_Key, "Alignment Type", "The type of alignment to perform (Origin or Centroid.", 0, std::vector<std::string>{"Origin", "Centroid"}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType AlignGeometriesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignGeometriesFilter::clone() const
{
  return std::make_unique<AlignGeometriesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignGeometriesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  OutputActions actions;
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> AlignGeometriesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  AlignGeometriesInputValues inputValues;
  inputValues.AlignmentTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_AlignmentType_Key);
  inputValues.InputMovingGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_MovingGeometry_Key);
  inputValues.InputTargetGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_TargetGeometry_Key);

  return AlignGeometries(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_AlignmentTypeKey = "AlignmentType";
constexpr StringLiteral k_MovingGeometryKey = "MovingGeometry";
constexpr StringLiteral k_TargetGeometryKey = "TargetGeometry";
} // namespace SIMPL
} // namespace

Result<Arguments> AlignGeometriesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AlignGeometriesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_AlignmentTypeKey, k_AlignmentType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_MovingGeometryKey, k_MovingGeometry_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_TargetGeometryKey, k_TargetGeometry_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
