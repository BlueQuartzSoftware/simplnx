#include "ComputeCoordinatesImageGeomFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeCoordinatesImageGeom.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeCoordinatesImageGeomFilter::name() const
{
  return FilterTraits<ComputeCoordinatesImageGeomFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeCoordinatesImageGeomFilter::className() const
{
  return FilterTraits<ComputeCoordinatesImageGeomFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeCoordinatesImageGeomFilter::uuid() const
{
  return FilterTraits<ComputeCoordinatesImageGeomFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeCoordinatesImageGeomFilter::humanName() const
{
  return "Compute Coordinates/Indices Array From Image Geom";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeCoordinatesImageGeomFilter::defaultTags() const
{
  return {className(), "stats", "statistics"};
}

//------------------------------------------------------------------------------
Parameters ComputeCoordinatesImageGeomFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_OutputType_Key, "Output Array(s) Type", "The selection here effects which arrays will be produced by the filter",
                                                                    to_underlying(ComputeCoordinatesImageGeom::OutputType::Physical),
                                                                    ChoicesParameter::Choices{"Physical Coordinates", "Indices", "Both"})); // sequence dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Selected Image Geometry",
                                                             "The DataPath to the Image Geometry that produced coordinates or indices will map to", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Output Data Array(s)"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CoordsArrayPath_Key, "Created Physical Coordinate Array Path",
                                                         "name and path of a new float32 DataArray containing the physical XYZ coordinate of the selected Image Geometry in global space",
                                                         DataPath({"Image Physical Coordinates"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_IndicesArrayPath_Key, "Created Indices Array Path",
                                                         "name and path of a new int32 DataArray containing the XYZ indices of the selected Image Geometry", DataPath({"Image Indices"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_OutputType_Key, k_CoordsArrayPath_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinatesImageGeom::OutputType::Physical)));

  params.linkParameters(k_OutputType_Key, k_IndicesArrayPath_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinatesImageGeom::OutputType::Index)));

  params.linkParameters(k_OutputType_Key, k_CoordsArrayPath_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinatesImageGeom::OutputType::Both)));
  params.linkParameters(k_OutputType_Key, k_IndicesArrayPath_Key, static_cast<ChoicesParameter::ValueType>(to_underlying(ComputeCoordinatesImageGeom::OutputType::Both)));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeCoordinatesImageGeomFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeCoordinatesImageGeomFilter::clone() const
{
  return std::make_unique<ComputeCoordinatesImageGeomFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeCoordinatesImageGeomFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pOutputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  auto pSelectedImageGeomValue = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedImageGeomPath_Key);
  auto pCoordsArrayPathValue = filterArgs.value<ArrayCreationParameter::ValueType>(k_CoordsArrayPath_Key);
  auto pIndicesArrayPathValue = filterArgs.value<ArrayCreationParameter::ValueType>(k_IndicesArrayPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  usize numberOfCells = dataStructure.getDataRefAs<ImageGeom>(pSelectedImageGeomValue).getNumberOfCells();

  if(pOutputTypeValue != to_underlying(ComputeCoordinatesImageGeom::OutputType::Index))
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{numberOfCells}, std::vector<usize>{3}, pCoordsArrayPathValue);
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  if(pOutputTypeValue != to_underlying(ComputeCoordinatesImageGeom::OutputType::Physical))
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{numberOfCells}, std::vector<usize>{3}, pIndicesArrayPathValue);
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ComputeCoordinatesImageGeomFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeCoordinatesImageGeomInputValues inputValues;

  inputValues.CoordinateOption = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  inputValues.ImageGeomPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedImageGeomPath_Key);
  inputValues.CoordArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_CoordsArrayPath_Key);
  inputValues.IndexArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_IndicesArrayPath_Key);

  return ComputeCoordinatesImageGeom(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
