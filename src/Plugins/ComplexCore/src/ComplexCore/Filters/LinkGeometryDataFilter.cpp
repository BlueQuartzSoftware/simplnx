#include "LinkGeometryDataFilter.hpp"

#include "ComplexCore/Filters/Algorithms/QuickSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateGeometry2DAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string LinkGeometryDataFilter::name() const
{
  return FilterTraits<LinkGeometryDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string LinkGeometryDataFilter::className() const
{
  return FilterTraits<LinkGeometryDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid LinkGeometryDataFilter::uuid() const
{
  return FilterTraits<LinkGeometryDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string LinkGeometryDataFilter::humanName() const
{
  return "Link Geometry Data Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> LinkGeometryDataFilter::defaultTags() const
{
  return {"#Geometry", "#DataArray Link"};
}

//------------------------------------------------------------------------------
Parameters LinkGeometryDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryDataPath_Key, "Selected Geometry", "The complete path to the Geometry with which to link the data", DataPath{},
                                                             IGeometry::GetAllGeomTypes()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedVertexDataArrayPaths_Key, "Vertex Data Arrays to Link", "Data associated with a vertex or point",
                                                               MultiArraySelectionParameter::ValueType{}, complex::GetAllDataTypes()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedEdgeDataArrayPaths_Key, "Edge Data Arrays to Link", "Data associated with an edge", MultiArraySelectionParameter::ValueType{},
                                                               complex::GetAllDataTypes()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedFaceDataArrayPaths_Key, "Face Data Arrays to Link", "Data associated with a face", MultiArraySelectionParameter::ValueType{},
                                                               complex::GetAllDataTypes()));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedVolumeDataArrayPaths_Key, "Cell Data Arrays to Link",
                                                               "Data associated with a cell or volume element such as a hexahedron or image geometry cell", MultiArraySelectionParameter::ValueType{},
                                                               complex::GetAllDataTypes()));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer LinkGeometryDataFilter::clone() const
{
  return std::make_unique<LinkGeometryDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult LinkGeometryDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto pGeomDataPath = filterArgs.value<DataPath>(k_GeometryDataPath_Key);
  auto pSelectedVertexDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedVertexDataArrayPaths_Key);
  auto pSelectedEdgeDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedEdgeDataArrayPaths_Key);
  auto pSelectedFaceDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFaceDataArrayPaths_Key);
  auto pSelectedVolumeDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedVolumeDataArrayPaths_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions = {};

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Assign the outputAction to the Result<OutputActions>::actions vector via a push_back
  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point, we are going to scope each section so that we don't accidentally introduce bugs

  // Collect all the errors
  std::vector<Error> errors;

  const auto* geometry = dataStructure.getDataAs<IGeometry>(pGeomDataPath);
  if(geometry == nullptr)
  {
    Error result = {-600, fmt::format("DataPath '{}' is not a Geometry", pGeomDataPath.toString())};
    errors.push_back(result);
  }
  // This could fail at runtime. we just do a basic sanity check here.
  for(const auto& selectedDataPath : pSelectedVertexDataArrayPaths)
  {
    if(!dataStructure.getId(selectedDataPath).has_value())
    {
      Error result = {-601, fmt::format("DataPath '{}' does not exist in the DataStructure", selectedDataPath.toString())};
      errors.push_back(result);
    }
  }
  for(const auto& selectedDataPath : pSelectedEdgeDataArrayPaths)
  {
    if(!dataStructure.getId(selectedDataPath).has_value())
    {
      Error result = {-601, fmt::format("DataPath '{}' does not exist in the DataStructure", selectedDataPath.toString())};
      errors.push_back(result);
    }
  }
  for(const auto& selectedDataPath : pSelectedFaceDataArrayPaths)
  {
    if(!dataStructure.getId(selectedDataPath).has_value())
    {
      Error result = {-601, fmt::format("DataPath '{}' does not exist in the DataStructure", selectedDataPath.toString())};
      errors.push_back(result);
    }
  }
  for(const auto& selectedDataPath : pSelectedVolumeDataArrayPaths)
  {
    if(!dataStructure.getId(selectedDataPath).has_value())
    {
      Error result = {-601, fmt::format("DataPath '{}' does not exist in the DataStructure", selectedDataPath.toString())};
      errors.push_back(result);
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> LinkGeometryDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  auto pGeomDataPath = filterArgs.value<DataPath>(k_GeometryDataPath_Key);
  auto pSelectedVertexDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedVertexDataArrayPaths_Key);
  auto pSelectedEdgeDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedEdgeDataArrayPaths_Key);
  auto pSelectedFaceDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedFaceDataArrayPaths_Key);
  auto pSelectedVolumeDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedVolumeDataArrayPaths_Key);

  auto& geometry = dataStructure.getDataRefAs<IGeometry>(pGeomDataPath);
  LinkedGeometryData& geometryData = geometry.getLinkedGeometryData();

  using IDataArrayShrdPtrType = std::shared_ptr<IDataArray>;

  for(const auto& selectedDataPath : pSelectedVertexDataArrayPaths)
  {
    //    IDataArrayShrdPtrType dataArray = dataStructure.getSharedDataAs<IDataArray>(selectedDataPath);
    //    DataObject::IdType dataId = dataStructure.getId(selectedDataPath).value();
    geometryData.addVertexData(selectedDataPath);
  }
  for(const auto& selectedDataPath : pSelectedEdgeDataArrayPaths)
  {
    //    IDataArrayShrdPtrType dataArray = dataStructure.getSharedDataAs<IDataArray>(selectedDataPath);
    //    DataObject::IdType dataId = dataStructure.getId(selectedDataPath).value();
    geometryData.addEdgeData(selectedDataPath);
  }
  for(const auto& selectedDataPath : pSelectedFaceDataArrayPaths)
  {
    //    IDataArrayShrdPtrType dataArray = dataStructure.getSharedDataAs<IDataArray>(selectedDataPath);
    //    DataObject::IdType dataId = dataStructure.getId(selectedDataPath).value();
    geometryData.addFaceData(selectedDataPath);
  }
  for(const auto& selectedDataPath : pSelectedVolumeDataArrayPaths)
  {
    //    IDataArrayShrdPtrType dataArray = dataStructure.getSharedDataAs<IDataArray>(selectedDataPath);
    //    DataObject::IdType dataId = dataStructure.getId(selectedDataPath).value();
    geometryData.addCellData(selectedDataPath);
  }

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  return {};
}
} // namespace complex
