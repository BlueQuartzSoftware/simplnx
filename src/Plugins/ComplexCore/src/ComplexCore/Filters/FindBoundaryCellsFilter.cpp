#include "FindBoundaryCellsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindBoundaryCells.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundaryCellsFilter::name() const
{
  return FilterTraits<FindBoundaryCellsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundaryCellsFilter::className() const
{
  return FilterTraits<FindBoundaryCellsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundaryCellsFilter::uuid() const
{
  return FilterTraits<FindBoundaryCellsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundaryCellsFilter::humanName() const
{
  return "Find Boundary Cells (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundaryCellsFilter::defaultTags() const
{
  return {"Generic", "Spatial"};
}

//------------------------------------------------------------------------------
Parameters FindBoundaryCellsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_IgnoreFeatureZero_Key, "Ignore Feature 0", "Do not use feature 0", false));
  params.insert(std::make_unique<BoolParameter>(k_IncludeVolumeBoundary_Key, "Include Volume Boundary", "Include the Cell boundaries", false));
  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The selected geometry to which the cells belong", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Cell belongs.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_BoundaryCellsArrayName_Key, "Boundary Cells",
                                                          "The number of neighboring Cells of a given Cell that belong to a different Feature than itself. Values will range from 0 to 6",
                                                          "BoundaryCells"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundaryCellsFilter::clone() const
{
  return std::make_unique<FindBoundaryCellsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindBoundaryCellsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pBoundaryCellsArrayNameValue = filterArgs.value<std::string>(k_BoundaryCellsArrayName_Key);

  const DataPath boundaryCellsArrayPath = pFeatureIdsArrayPathValue.getParent().createChildPath(pBoundaryCellsArrayNameValue);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  const usize numFeatureIds = featureIds.getSize();
  const usize numCells = dataStructure.getDataRefAs<ImageGeom>(pImageGeometryPath).getNumberOfCells();
  if(numFeatureIds != numCells)
  {
    return {MakeErrorResult<OutputActions>(
        -5320,
        fmt::format("The selected Image geometry '{}' and feature ids array '{}' have mismatching number of elements. Make sure the selected feature ids was created for the selected Image geometry.",
                    pImageGeometryPath.toString(), pFeatureIdsArrayPathValue.toString()))};
  }

  auto action = std::make_unique<CreateArrayAction>(DataType::int8, featureIds.getTupleShape(), std::vector<usize>{1}, boundaryCellsArrayPath);
  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindBoundaryCellsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  FindBoundaryCellsInputValues inputValues;

  inputValues.IgnoreFeatureZero = filterArgs.value<bool>(k_IgnoreFeatureZero_Key);
  inputValues.IncludeVolumeBoundary = filterArgs.value<bool>(k_IncludeVolumeBoundary_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.BoundaryCellsArrayName = inputValues.FeatureIdsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_BoundaryCellsArrayName_Key));

  return FindBoundaryCells(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
