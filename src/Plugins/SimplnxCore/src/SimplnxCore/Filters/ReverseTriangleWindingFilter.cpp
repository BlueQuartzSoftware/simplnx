#include "ReverseTriangleWindingFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReverseTriangleWinding.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReverseTriangleWindingFilter::name() const
{
  return FilterTraits<ReverseTriangleWindingFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReverseTriangleWindingFilter::className() const
{
  return FilterTraits<ReverseTriangleWindingFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReverseTriangleWindingFilter::uuid() const
{
  return FilterTraits<ReverseTriangleWindingFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReverseTriangleWindingFilter::humanName() const
{
  return "Reverse Triangle Winding";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReverseTriangleWindingFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Connectivity Arrangement"};
}

//------------------------------------------------------------------------------
Parameters ReverseTriangleWindingFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeomPath_Key, "Triangle Geometry", "The DataPath to then input Triangle Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReverseTriangleWindingFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReverseTriangleWindingFilter::clone() const
{
  return std::make_unique<ReverseTriangleWindingFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReverseTriangleWindingFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pTriGeomPathValue = filterArgs.value<DataPath>(k_TriGeomPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  // Inform users that the following arrays are going to be modified in place
  // Triangle geometry face list is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pTriGeomPathValue, {});

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ReverseTriangleWindingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReverseTriangleWindingInputValues inputValues;
  inputValues.InputTriangleGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_TriGeomPath_Key);

  return ReverseTriangleWinding(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceDataContainerNameKey = "SurfaceDataContainerName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReverseTriangleWindingFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReverseTriangleWindingFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceDataContainerNameKey, k_TriGeomPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
