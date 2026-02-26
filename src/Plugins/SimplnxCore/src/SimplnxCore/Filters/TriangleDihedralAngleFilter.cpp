#include "TriangleDihedralAngleFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/TriangleDihedralAngle.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
constexpr nx::core::int32 k_MissingFeatureAttributeMatrix = -76970;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::name() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::className() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::className;
}

//------------------------------------------------------------------------------
Uuid TriangleDihedralAngleFilter::uuid() const
{
  return FilterTraits<TriangleDihedralAngleFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string TriangleDihedralAngleFilter::humanName() const
{
  return "Calculate Triangle Minimum Dihedral Angle";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleDihedralAngleFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Misc", "Statistics", "Triangle"};
}

//------------------------------------------------------------------------------
Parameters TriangleDihedralAngleFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the dihedral angles", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshTriangleDihedralAnglesArrayName_Key, "Created Dihedral Angles", "The name of the array storing the calculated dihedral angles",
                                                          "Dihedral Angles"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType TriangleDihedralAngleFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TriangleDihedralAngleFilter::clone() const
{
  return std::make_unique<TriangleDihedralAngleFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult TriangleDihedralAngleFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TGeometryDataPath_Key);
  auto pMinDihedralAnglesName = filterArgs.value<std::string>(k_SurfaceMeshTriangleDihedralAnglesArrayName_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(pTriangleGeometryDataPath);
  // Get the Face AttributeMatrix from the Geometry (It should have been set at construction of the Triangle Geometry)
  const AttributeMatrix* faceAttributeMatrix = triangleGeom.getFaceAttributeMatrix();
  if(faceAttributeMatrix == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-9861, fmt::format("Cannot find the face data Attribute Matrix for the selected Triangle Geometry at path '{}'", pTriangleGeometryDataPath.toString()))};
  }

  // Instantiate and move the action that will create the output array
  {
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pMinDihedralAnglesName);
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float64, std::vector<usize>{triangleGeom.getNumberOfFaces()}, std::vector<usize>{1}, createArrayDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> TriangleDihedralAngleFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  TriangleDihedralAngleInputValues inputValues;
  inputValues.InputTriangleGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_TGeometryDataPath_Key);
  inputValues.SurfaceMeshTriangleDihedralAnglesArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SurfaceMeshTriangleDihedralAnglesArrayName_Key);

  return TriangleDihedralAngle(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceMeshTriangleDihedralAnglesArrayPathKey = "SurfaceMeshTriangleDihedralAnglesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> TriangleDihedralAngleFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = TriangleDihedralAngleFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshTriangleDihedralAnglesArrayPathKey,
                                                                                                                                  k_SurfaceMeshTriangleDihedralAnglesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(
      args, json, SIMPL::k_SurfaceMeshTriangleDihedralAnglesArrayPathKey, k_TGeometryDataPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
