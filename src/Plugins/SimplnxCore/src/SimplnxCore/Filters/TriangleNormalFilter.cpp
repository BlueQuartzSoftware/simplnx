#include "TriangleNormalFilter.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{

constexpr nx::core::int32 k_MissingFeatureAttributeMatrix = -75969;

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string TriangleNormalFilter::name() const
{
  return FilterTraits<TriangleNormalFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string TriangleNormalFilter::className() const
{
  return FilterTraits<TriangleNormalFilter>::className;
}

//------------------------------------------------------------------------------
Uuid TriangleNormalFilter::uuid() const
{
  return FilterTraits<TriangleNormalFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string TriangleNormalFilter::humanName() const
{
  return "Compute Triangle Normals";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleNormalFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Misc"};
}

//------------------------------------------------------------------------------
Parameters TriangleNormalFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the normals", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_SurfaceMeshTriangleNormalsArrayName_Key, "Created Face Normals", "The complete path to the array storing the calculated normals", "Face Normals"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType TriangleNormalFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TriangleNormalFilter::clone() const
{
  return std::make_unique<TriangleNormalFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult TriangleNormalFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  auto pNormalsArrayName = filterArgs.value<std::string>(k_SurfaceMeshTriangleNormalsArrayName_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  // Get the Face AttributeMatrix from the Geometry (It should have been set at construction of the Triangle Geometry)
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();
  if(faceAttributeMatrix == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{
        Error{k_MissingFeatureAttributeMatrix, fmt::format("Could not find Triangle Face Attribute Matrix with in the Triangle Geometry '{}'", pTriangleGeometryDataPath.toString())}})};
  }
  // Instantiate and move the action that will create the output array
  {
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pNormalsArrayName);
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{3}, createArrayDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> TriangleNormalFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  auto pNormalsName = filterArgs.value<std::string>(k_SurfaceMeshTriangleNormalsArrayName_Key);

  const TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();

  DataPath pNormalsArrayPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pNormalsName);
  auto& normals = dataStructure.getDataAs<Float64Array>(pNormalsArrayPath)->getDataStoreRef();

  return nx::core::GeometryUtilities::ComputeTriangleNormals(triangleGeom, normals, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceMeshTriangleNormalsArrayPathKey = "SurfaceMeshTriangleNormalsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> TriangleNormalFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = TriangleNormalFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshTriangleNormalsArrayPathKey, k_TriGeometryDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshTriangleNormalsArrayPathKey,
                                                                                                              k_SurfaceMeshTriangleNormalsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
