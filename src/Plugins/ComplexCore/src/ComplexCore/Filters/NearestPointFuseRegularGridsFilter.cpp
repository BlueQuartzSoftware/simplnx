#include "NearestPointFuseRegularGridsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/NearestPointFuseRegularGrids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Filter/Actions/CreateStringArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

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
  return {className(), "Sampling", "Spacing"};
}

//------------------------------------------------------------------------------
Parameters NearestPointFuseRegularGridsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseFill_Key, "Use Custom Fill Value", "If false all copied arrays will be filled with 0 by default", false));
  params.insert(std::make_unique<NumberParameter<float64>>(k_FillValue_Key, "Fill Value", "This is the value that will appear in the arrays outside the overlap", 0.0));

  params.insertSeparator(Parameters::Separator{"Required Sampling Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SamplingGeometryPath_Key, "Sampling Image Geometry",
                                                             "This is the geometry that will be copied into the reference geometry at the overlap", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SamplingCellAttributeMatrixPath_Key, "Sampling Cell Attribute Matrix", "The attribute matrix for the sampling geometry", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));

  params.insertSeparator(Parameters::Separator{"Required Reference Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ReferenceGeometryPath_Key, "Reference Image Geometry", "This is the geometry that will store the values from the overlap", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_ReferenceCellAttributeMatrixPath_Key, "Reference Cell Attribute Matrix", "The attribute matrix for the reference geometry", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));

  // link parameters
  params.linkParameters(k_UseFill_Key, k_FillValue_Key, true);

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
  {
    auto sampleVoxelArrays = sampleAM->findAllChildrenOfType<IDataArray>();
    for(const auto& array : sampleVoxelArrays)
    {
      DataPath createdArrayPath = pReferenceCellAttributeMatrixPathValue.createChildPath(array->getName());
      auto createArrayAction = std::make_unique<CreateArrayAction>(array->getDataType(), refAM->getShape(), array->getComponentShape(), createdArrayPath);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  {
    auto sampleVoxelArrays = sampleAM->findAllChildrenOfType<StringArray>();
    for(const auto& array : sampleVoxelArrays)
    {
      DataPath createdArrayPath = pReferenceCellAttributeMatrixPathValue.createChildPath(array->getName());
      auto createArrayAction = std::make_unique<CreateStringArrayAction>(refAM->getShape(), createdArrayPath);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  // !!!! Not implemented for v1 !!!!
  //  {
  //    auto sampleVoxelArrays = sampleAM->findAllChildrenOfType<INeighborList>();
  //    for(const auto& array : sampleVoxelArrays)
  //    {
  //      DataPath createdArrayPath = pReferenceCellAttributeMatrixPathValue.createChildPath(array->getName());
  //      auto createArrayAction = std::make_unique<CreateNeighborListAction>(array->getDataType(), array->getNumberOfTuples(), createdArrayPath);
  //      resultOutputActions.value().appendAction(std::move(createArrayAction));
  //    }
  //  }

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
  filterArgs.value<bool>(k_UseFill_Key) ? inputValues.fillValue = filterArgs.value<float64>(k_FillValue_Key) : inputValues.fillValue = 0.0;

  return NearestPointFuseRegularGrids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
