#include "NearestPointFuseRegularGridsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/NearestPointFuseRegularGrids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string NearestPointFuseRegularGridsFilter::name() const
{
  return FilterTraits<NearestPointFuseRegularGridsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string NearestPointFuseRegularGridsFilter::className() const
{
  return FilterTraits<NearestPointFuseRegularGridsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid NearestPointFuseRegularGridsFilter::uuid() const
{
  return FilterTraits<NearestPointFuseRegularGridsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string NearestPointFuseRegularGridsFilter::humanName() const
{
  return "Fuse Regular Grids (Nearest Point)";
}

//------------------------------------------------------------------------------
std::vector<std::string> NearestPointFuseRegularGridsFilter::defaultTags() const
{
  return {"Sampling", "Spacing"};
}

//------------------------------------------------------------------------------
Parameters NearestPointFuseRegularGridsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SamplingGeometryPath_Key, "Sampling Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_ReferenceGeometryPath_Key, "Reference Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SamplingCellAttributeMatrixPath_Key, "Sampling Cell Attribute Matrix", "", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ReferenceCellAttributeMatrixPath_Key, "Reference Cell Attribute Matrix", "", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer NearestPointFuseRegularGridsFilter::clone() const
{
  return std::make_unique<NearestPointFuseRegularGridsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult NearestPointFuseRegularGridsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pSamplingGeometryPathValue = filterArgs.value<DataPath>(k_SamplingGeometryPath_Key);
  auto pReferenceGeometryPathValue = filterArgs.value<DataPath>(k_ReferenceGeometryPath_Key);
  auto pSamplingCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SamplingCellAttributeMatrixPath_Key);
  auto pReferenceCellAttributeMatrixPathValue = filterArgs.value<DataPath>(k_ReferenceCellAttributeMatrixPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto* sampleAM = dataStructure.getDataAs<AttributeMatrix>(pSamplingCellAttributeMatrixPathValue);
  auto* refAM = dataStructure.getDataAs<AttributeMatrix>(pReferenceCellAttributeMatrixPathValue);

  // Create arrays on the reference grid to hold data present on the sampling grid
  auto sampleVoxelArrays = sampleAM->findAllChildrenOfType<IDataArray>();
  for(const auto& array : sampleVoxelArrays)
  {
    DataPath createdArrayPath = pReferenceCellAttributeMatrixPathValue.createChildPath(array->getName());
    auto createArrayAction = std::make_unique<CreateArrayAction>(array->getDataType(), refAM->getShape(), array->getComponentShape(), createdArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> NearestPointFuseRegularGridsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  NearestPointFuseRegularGridsInputValues inputValues;

  inputValues.SamplingGeometryPath = filterArgs.value<DataPath>(k_SamplingGeometryPath_Key);
  inputValues.ReferenceGeometryPath = filterArgs.value<DataPath>(k_ReferenceGeometryPath_Key);
  inputValues.SamplingCellAttributeMatrixPath = filterArgs.value<DataPath>(k_SamplingCellAttributeMatrixPath_Key);
  inputValues.ReferenceCellAttributeMatrixPath = filterArgs.value<DataPath>(k_ReferenceCellAttributeMatrixPath_Key);

  return NearestPointFuseRegularGrids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
