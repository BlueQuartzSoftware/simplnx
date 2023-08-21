#include "TriangleCentroidFilter.hpp"

#include "ComplexCore/Filters/Algorithms/TriangleCentroid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;
namespace
{

constexpr complex::int32 k_MissingFeatureAttributeMatrix = -75969;

}
namespace complex
{
//------------------------------------------------------------------------------
std::string TriangleCentroidFilter::name() const
{
  return FilterTraits<TriangleCentroidFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string TriangleCentroidFilter::className() const
{
  return FilterTraits<TriangleCentroidFilter>::className;
}

//------------------------------------------------------------------------------
Uuid TriangleCentroidFilter::uuid() const
{
  return FilterTraits<TriangleCentroidFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string TriangleCentroidFilter::humanName() const
{
  return "Calculate Triangle Centroids";
}

//------------------------------------------------------------------------------
std::vector<std::string> TriangleCentroidFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Misc"};
}

//------------------------------------------------------------------------------
Parameters TriangleCentroidFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the normals", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Created Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CentroidsArrayName_Key, "Created Face Centroids", "The complete path to the array storing the calculated centroids", "Centroids"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer TriangleCentroidFilter::clone() const
{
  return std::make_unique<TriangleCentroidFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult TriangleCentroidFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{

  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  auto pCentroidsArrayName = filterArgs.value<std::string>(k_CentroidsArrayName_Key);

  std::vector<PreflightValue> preflightUpdatedValues;

  complex::Result<OutputActions> resultOutputActions;

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
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pCentroidsArrayName);
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{3}, createArrayDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> TriangleCentroidFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{

  TriangleCentroidInputValues inputValues;

  inputValues.TriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriGeometryDataPath_Key);
  inputValues.CentroidsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CentroidsArrayName_Key);

  return TriangleCentroid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
