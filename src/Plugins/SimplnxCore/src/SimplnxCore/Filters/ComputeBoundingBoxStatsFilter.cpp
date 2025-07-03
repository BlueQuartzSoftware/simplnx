#include "ComputeBoundingBoxStatsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeBoundingBoxStats.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeBoundingBoxStatsFilter::name() const
{
  return FilterTraits<ComputeBoundingBoxStatsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeBoundingBoxStatsFilter::className() const
{
  return FilterTraits<ComputeBoundingBoxStatsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeBoundingBoxStatsFilter::uuid() const
{
  return FilterTraits<ComputeBoundingBoxStatsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeBoundingBoxStatsFilter::humanName() const
{
  return "Compute Statistics Within Bounding Boxes";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeBoundingBoxStatsFilter::defaultTags() const
{
  return {className(), "Statistics"};
}

//------------------------------------------------------------------------------
Parameters ComputeBoundingBoxStatsFilter::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Selected Image Geometry", "The DataPath to the Geometry that contains the points/edges/faces for the geometry",
                                                             DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeBoundingBoxStatsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeBoundingBoxStatsFilter::clone() const
{
  return std::make_unique<ComputeBoundingBoxStatsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeBoundingBoxStatsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {};
}

//------------------------------------------------------------------------------
Result<> ComputeBoundingBoxStatsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeBoundingBoxStatsInputValues inputValues;

  inputValues.CalculateLength = filterArgs.value<bool>(k_CalculateLength_Key);
  inputValues.CalculateMin = filterArgs.value<bool>(k_CalculateMin_Key);
  inputValues.CalculateMax = filterArgs.value<bool>(k_CalculateMax_Key);
  inputValues.CalculateSummation = filterArgs.value<bool>(k_CalculateSummation_Key);
  inputValues.CalculateMean = filterArgs.value<bool>(k_CalculateMean_Key);
  inputValues.CalculateMode = filterArgs.value<bool>(k_CalculateMode_Key);
  inputValues.CalculateStdDev = filterArgs.value<bool>(k_CalculateStandardDeviation_Key);

  inputValues.GeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.UnifiedPath = filterArgs.value<DataPath>(k_UnifiedBoundsPath_Key);
  inputValues.InputPath = filterArgs.value<DataPath>(k_InputArrayPath_Key);

  DataPath outputAMPath = {};

  inputValues.BoundsHasDataPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_BoundsHasDataName_Key));
  inputValues.LengthPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_LengthName_Key));
  inputValues.MinPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MinName_Key));
  inputValues.MaxPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MaxName_Key));
  inputValues.SummationPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_SummationName_Key));
  inputValues.MeanPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_MeanName_Key));
  inputValues.ModePath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_ModeName_Key));
  inputValues.StdDevPath = outputAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_StdDevName_Key));

  return ComputeBoundingBoxStats(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
