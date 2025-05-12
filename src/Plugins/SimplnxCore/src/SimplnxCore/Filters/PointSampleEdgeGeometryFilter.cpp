#include "PointSampleEdgeGeometryFilter.hpp"

#include "simplnx/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "SimplnxCore/Filters/Algorithms/PointSampleEdgeGeometry.hpp"

using namespace nx::core;

namespace
{
} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string PointSampleEdgeGeometryFilter::name() const
{
  return FilterTraits<PointSampleEdgeGeometryFilter>::name;
}

//------------------------------------------------------------------------------
std::string PointSampleEdgeGeometryFilter::className() const
{
  return FilterTraits<PointSampleEdgeGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid PointSampleEdgeGeometryFilter::uuid() const
{
  return FilterTraits<PointSampleEdgeGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string PointSampleEdgeGeometryFilter::humanName() const
{
  return "Point Sample Edge Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> PointSampleEdgeGeometryFilter::defaultTags() const
{
  return {className(), "edge", "vertex", "sampling", "interpolation", "resolution"};
}

//------------------------------------------------------------------------------
Parameters PointSampleEdgeGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(
      std::make_unique<NumberParameter<float32>>(k_ScanVectorSamplingRes_Key, "Sampling Spacing (mm)", "Distance (in millimeters) between successive sample points along each scan vector.", 0.1f));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ScanVectorGeometryPath_Key, "Input Edge Geometry", "Path to the Edge geometry along which sampling will occur.", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Edge}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_CalculateCumulativeSampleDistance_Key, "Calculate Cumulative Sample Distance",
      "Determines whether or not to calculate the cumulative distance (the distance from the starting endpoint to the sample vertex's location) for each sample vertex on each edge.", true));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Edge Attribute Arrays to Transfer",
                                                               "The paths to the Edge Attribute Arrays to transfer to the created Vertex Geometry", MultiArraySelectionParameter::ValueType{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Parameter(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_SampledVertexGeometryPath_Key, "Output Vertex Geometry",
                                                             "Location in the data structure where the new Vertex Geometry, containing all points sampled along each scan vector, will be created.",
                                                             DataPath({"Sampled Vertex Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataGroupName_Key, "Created Vertex Data Attribute Matrix Name", "The name of the created Vertex Attribute Matrix",
                                                          INodeGeometry0D::k_VertexAttributeMatrixName));

  params.insert(std::make_unique<DataObjectNameParameter>(k_EdgeIdsArrayName_Key, "Output Edge Ids Array Name",
                                                          "Name for the array that will hold, for each sampled vertex, the id of the edge that it belongs to.", "Edge Ids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CumulativeSampleDistanceArrayName_Key, "Output Cumulative Sample Distance Array Name",
                                                          "Name for the array that will hold, for each sampled vertex, the linear distance from the beginning of its scan vector.",
                                                          "Cumulative Sample Distance"));

  params.linkParameters(k_CalculateCumulativeSampleDistance_Key, k_CumulativeSampleDistanceArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType PointSampleEdgeGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PointSampleEdgeGeometryFilter::clone() const
{
  return std::make_unique<PointSampleEdgeGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PointSampleEdgeGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pVertexGeometryDataPath = filterArgs.value<DataPath>(k_SampledVertexGeometryPath_Key);
  auto pVertexGroupDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  DataPath pVertexGroupDataPath = pVertexGeometryDataPath.createChildPath(pVertexGroupDataName);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto cumulativeSampleDistArrayName = filterArgs.value<std::string>(k_CumulativeSampleDistanceArrayName_Key);
  auto edgeIdsArrayName = filterArgs.value<std::string>(k_EdgeIdsArrayName_Key);
  auto calculateCumulativeSampleDist = filterArgs.value<bool>(k_CalculateCumulativeSampleDistance_Key);

  nx::core::Result<OutputActions> resultOutputActions = {};

  std::vector<PreflightValue> preflightUpdatedValues;

  // Create the Vertex Geometry action and store it
  {
    auto createVertexGeometryAction = std::make_unique<CreateVertexGeometryAction>(pVertexGeometryDataPath, 1, pVertexGroupDataName, VertexGeom::k_SharedVertexListName);
    resultOutputActions.value().appendAction(std::move(createVertexGeometryAction));
  }

  const std::vector<usize>& tDims = {1ULL};

  // Create the output EdgeIds Array
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint64, tDims, std::vector<usize>{1}, pVertexGroupDataPath.createChildPath(edgeIdsArrayName)));

  // Create all the target DataArray based on the Selected Node Arrays
  for(const auto& selectedDataPath : pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = pVertexGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    const auto& selectedDataArray = dataStructure.getDataRefAs<IDataArray>(selectedDataPath);
    DataType dataType = selectedDataArray.getDataType();
    auto createArrayAction = std::make_unique<CreateArrayAction>(dataType, tDims, selectedDataArray.getComponentShape(), createdDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  if(calculateCumulativeSampleDist)
  {
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{1}, std::vector<usize>{1}, pVertexGroupDataPath.createChildPath(cumulativeSampleDistArrayName)));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues  LinkGeometryDataFilter via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> PointSampleEdgeGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  PointSampleEdgeGeometryInputValues inputs;
  inputs.ScanVectorSamplingRes = filterArgs.value<float32>(k_ScanVectorSamplingRes_Key);
  inputs.ScanVectorGeometryPath = filterArgs.value<DataPath>(k_ScanVectorGeometryPath_Key);
  inputs.SampledVertexGeometryPath = filterArgs.value<DataPath>(k_SampledVertexGeometryPath_Key);
  inputs.CumulativeSampleDistanceArrayName = filterArgs.value<std::string>(k_CumulativeSampleDistanceArrayName_Key);
  inputs.EdgeIdsArrayName = filterArgs.value<std::string>(k_EdgeIdsArrayName_Key);
  inputs.CalculateCumulativeSampleDistance = filterArgs.value<bool>(k_CalculateCumulativeSampleDistance_Key);
  inputs.pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pVertexGroupDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  inputs.pVertexGroupDataPath = inputs.ScanVectorGeometryPath.createChildPath(pVertexGroupDataName);

  MultiArraySelectionParameter::ValueType createdDataPaths;
  for(const auto& selectedDataPath : inputs.pSelectedDataArrayPaths)
  {
    createdDataPaths.push_back(inputs.pVertexGroupDataPath.createChildPath(selectedDataPath.getTargetName()));
  }
  inputs.pCreatedDataArrayPaths = createdDataPaths;

  return PointSampleEdgeGeometry(dataStructure, &inputs, messageHandler, shouldCancel)();
}
} // namespace nx::core
