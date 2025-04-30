#include "SampleScanVectorsFilter.hpp"

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "SimplnxCore/Filters/Algorithms/SampleScanVectors.hpp"

using namespace nx::core;

namespace
{
} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string SampleScanVectorsFilter::name() const
{
  return FilterTraits<SampleScanVectorsFilter>::name;
}

//------------------------------------------------------------------------------
std::string SampleScanVectorsFilter::className() const
{
  return FilterTraits<SampleScanVectorsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid SampleScanVectorsFilter::uuid() const
{
  return FilterTraits<SampleScanVectorsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string SampleScanVectorsFilter::humanName() const
{
  return "Sample Scan Vectors";
}

//------------------------------------------------------------------------------
std::vector<std::string> SampleScanVectorsFilter::defaultTags() const
{
  return {className(), "edge", "vertex", "sampling", "interpolation", "resolution"};
}

//------------------------------------------------------------------------------
Parameters SampleScanVectorsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<NumberParameter<float32>>(k_ScanVectorSamplingRes_Key, "Scan Vector Sampling Resolution (mm)",
                                                           "Distance (in millimeters) between successive sample points along each scan vector.", 0.1f));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ScanVectorGeometryPath_Key, "Scan Vector Geometry",
                                                             "Path to the Edge geometry representing scan vectors along which sampling will occur.", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Edge}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_CopyPowerData_Key, "Copy Power Data", "Determines whether or not to copy Power data from edges to the sampled vertices.", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_CopySliceIdData_Key, "Copy Slice Id Data", "Determines whether or not to copy Slice Id data from edges to the sampled vertices.", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_CopyEdgeIdData_Key, "Copy Edge Id Data", "Determines whether or not to copy the id of the edge that each sampled vertex lies on.", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_CalculateCumulativeSampleDistance_Key, "Calculate Cumulative Sample Distance",
      "Determines whether or not to calculate the cumulative distance (the distance from the starting endpoint to the sample vertex's location) for each sample vertex on each edge.", true));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PowerArrayPath_Key, "Power Array", "Array of power values for each scan vector; these will be assigned to every sampled point.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SliceIdArrayPath_Key, "Slice Id Array", "Array of slice identifiers for each scan vector, used to label sampled points by slice.",
                                                          DataPath(), ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Parameter(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_SampledVertexGeometryPath_Key, "Sampled Vertex Geometry",
                                                             "Location in the data structure where the new Vertex Geometry, containing all points sampled along each scan vector, will be created.",
                                                             DataPath({"Sampled Vertex Geometry"})));

  params.insert(std::make_unique<DataObjectNameParameter>(k_EdgeIdsArrayName_Key, "Edge Ids Array Name",
                                                          "Name for the array that will hold, for each sampled vertex, the id of the edge that it belongs to.", "Edge Ids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CumulativeSampleDistanceArrayName_Key, "Cumulative Sample Distance Array Name",
                                                          "Name for the array that will hold, for each sampled vertex, the linear distance from the beginning of its scan vector.",
                                                          "Cumulative Sample Distance"));

  params.linkParameters(k_CopyPowerData_Key, k_PowerArrayPath_Key, true);
  params.linkParameters(k_CopySliceIdData_Key, k_SliceIdArrayPath_Key, true);
  params.linkParameters(k_CopyEdgeIdData_Key, k_EdgeIdsArrayName_Key, true);
  params.linkParameters(k_CalculateCumulativeSampleDistance_Key, k_CumulativeSampleDistanceArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType SampleScanVectorsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SampleScanVectorsFilter::clone() const
{
  return std::make_unique<SampleScanVectorsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult SampleScanVectorsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto sampledVertexGeomPath = filterArgs.value<DataPath>(k_SampledVertexGeometryPath_Key);
  auto powerArrayPath = filterArgs.value<DataPath>(k_PowerArrayPath_Key);
  auto sliceIdArrayPath = filterArgs.value<DataPath>(k_SliceIdArrayPath_Key);
  auto cumulativeSampleDistArrayName = filterArgs.value<std::string>(k_CumulativeSampleDistanceArrayName_Key);
  auto edgeIdsArrayName = filterArgs.value<std::string>(k_EdgeIdsArrayName_Key);
  auto copyPowerData = filterArgs.value<bool>(k_CopyPowerData_Key);
  auto copySliceIds = filterArgs.value<bool>(k_CopySliceIdData_Key);
  auto copyEdgeIds = filterArgs.value<bool>(k_CopyEdgeIdData_Key);
  auto calculateCumulativeSampleDist = filterArgs.value<bool>(k_CalculateCumulativeSampleDistance_Key);

  OutputActions actions;

  if(!copyPowerData && !copySliceIds && !copyEdgeIds && !calculateCumulativeSampleDist)
  {
    return {MakeErrorResult<OutputActions>(-5320, "At least one of the copy/calculate booleans must be turned on.")};
  }

  actions.appendAction(std::make_unique<CreateVertexGeometryAction>(sampledVertexGeomPath, 1, VertexGeom::k_VertexAttributeMatrixName, VertexGeom::k_SharedVertexListName));

  if(copyPowerData)
  {
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{1}, std::vector<usize>{1},
                                                             sampledVertexGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(powerArrayPath.getTargetName())));
  }
  if(copySliceIds)
  {
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{1}, std::vector<usize>{1},
                                                             sampledVertexGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(sliceIdArrayPath.getTargetName())));
  }
  if(copyEdgeIds)
  {
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1},
                                                             sampledVertexGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(edgeIdsArrayName)));
  }
  if(calculateCumulativeSampleDist)
  {
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{1}, std::vector<usize>{1},
                                                             sampledVertexGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName).createChildPath(cumulativeSampleDistArrayName)));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> SampleScanVectorsFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  SampleScanVectorsInputValues inputValues;
  inputValues.ScanVectorSamplingRes = args.value<float32>(k_ScanVectorSamplingRes_Key);
  inputValues.ScanVectorGeometryPath = args.value<DataPath>(k_ScanVectorGeometryPath_Key);
  inputValues.PowerArrayPath = args.value<DataPath>(k_PowerArrayPath_Key);
  inputValues.SliceIdArrayPath = args.value<DataPath>(k_SliceIdArrayPath_Key);
  inputValues.SampledVertexGeometryPath = args.value<DataPath>(k_SampledVertexGeometryPath_Key);
  inputValues.CumulativeSampleDistanceArrayName = args.value<std::string>(k_CumulativeSampleDistanceArrayName_Key);
  inputValues.EdgeIdsArrayName = args.value<std::string>(k_EdgeIdsArrayName_Key);
  inputValues.CopyPowerData = args.value<bool>(k_CopyPowerData_Key);
  inputValues.CopySliceIds = args.value<bool>(k_CopySliceIdData_Key);
  inputValues.CopyEdgeIds = args.value<bool>(k_CopyEdgeIdData_Key);
  inputValues.CalculateCumulativeSampleDistance = args.value<bool>(k_CalculateCumulativeSampleDistance_Key);

  return SampleScanVectors(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
