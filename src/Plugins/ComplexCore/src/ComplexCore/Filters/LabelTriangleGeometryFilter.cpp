#include "LabelTriangleGeometryFilter.hpp"

#include "ComplexCore/Filters/Algorithms/LabelTriangleGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string LabelTriangleGeometryFilter::name() const
{
  return FilterTraits<LabelTriangleGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string LabelTriangleGeometryFilter::className() const
{
  return FilterTraits<LabelTriangleGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid LabelTriangleGeometryFilter::uuid() const
{
  return FilterTraits<LabelTriangleGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string LabelTriangleGeometryFilter::humanName() const
{
  return "Label CAD Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> LabelTriangleGeometryFilter::defaultTags() const
{
  return {"Surface Meshing", "Mapping"};
}

//------------------------------------------------------------------------------
Parameters LabelTriangleGeometryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeomPath_Key, "Triangle Geometry", "The CAD Geometry to be labeled (TriangleGeom)", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{GeometrySelectionParameter::AllowedType::Triangle}));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedRegionIdsPath_Key, "Region Ids", "The triangle id array for indexing into triangle groupings", DataPath({"Region Ids"})));

  params.insert(std::make_unique<DataObjectNameParameter>(k_TriangleAttributeMatrixName_Key, "Cell Feature Attribute Matrix Name",
                                                          "The name of the Cell Feature Attribute Matrix that will hold information arrays about each group; created in the CAD geometry",
                                                          "Cell Feature AM"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumTrianglesName_Key, "Number of Triangles Array Name",
                                                          "The name of array created in the new AM that stores the number of triangles per group", "NumTriangles"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer LabelTriangleGeometryFilter::clone() const
{
  return std::make_unique<LabelTriangleGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult LabelTriangleGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pTriangleGeomPathValue = filterArgs.value<DataPath>(k_TriangleGeomPath_Key);
  auto pRegionIdsNameValue = filterArgs.value<DataPath>(k_CreatedRegionIdsPath_Key);
  auto pTriangleAMNameValue = filterArgs.value<std::string>(k_TriangleAttributeMatrixName_Key);
  auto pNumTrianglesNameValue = filterArgs.value<std::string>(k_NumTrianglesName_Key);

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    const auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeomPathValue);
    if(triangleGeom == nullptr)
    {
      return MakePreflightErrorResult(-34510, fmt::format("{} is not a valid Triangle Geometry", pTriangleGeomPathValue.toString()));
    }
    usize numTris = triangleGeom->getNumberOfFaces();
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{numTris}, std::vector<usize>{1}, pRegionIdsNameValue);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  DataPath triAMPath = pTriangleGeomPathValue.createChildPath(pNumTrianglesNameValue);
  std::vector<usize> tDims(1, 1);
  {
    auto createAMAction = std::make_unique<CreateAttributeMatrixAction>(triAMPath, tDims);
    resultOutputActions.value().appendAction(std::move(createAMAction));
  }

  {
    DataPath numTrianglesPath = triAMPath.createChildPath(pNumTrianglesNameValue);
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint64, tDims, std::vector<usize>{1}, numTrianglesPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> LabelTriangleGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  LabelTriangleGeometryInputValues inputValues;

  inputValues.TriangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeomPath_Key);
  inputValues.RegionIdsPath = filterArgs.value<DataPath>(k_CreatedRegionIdsPath_Key);
  inputValues.TriangleAMPath = inputValues.TriangleGeomPath.createChildPath(filterArgs.value<std::string>(k_TriangleAttributeMatrixName_Key));
  inputValues.NumTrianglesPath = inputValues.TriangleAMPath.createChildPath(filterArgs.value<std::string>(k_NumTrianglesName_Key));

  return LabelTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
