#include "ComputeFeatureRectFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ComputeFeatureRect.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/IGridGeometry.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ComputeFeatureRectFilter::name() const
{
  return FilterTraits<ComputeFeatureRectFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureRectFilter::className() const
{
  return FilterTraits<ComputeFeatureRectFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureRectFilter::uuid() const
{
  return FilterTraits<ComputeFeatureRectFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureRectFilter::humanName() const
{
  return "Compute Feature Corners";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureRectFilter::defaultTags() const
{
  return {"#Statistics", "#Reconstruction", "#Rect", "#Calculate"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureRectFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "The feature ids array used to calculate the feature rect", DataPath{{"FeatureIds"}},
                                                          ArraySelectionParameter::AllowedTypes{complex::DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureRectArrayPath_Key, "Feature Rect", "The feature rect calculated from the feature ids", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureRectFilter::clone() const
{
  return std::make_unique<ComputeFeatureRectFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureRectFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureRectArrayPathValue = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);

  DataPath parentPath = pFeatureRectArrayPathValue.getParent();

  std::vector<usize> tDims = {0};
  if(dataStructure.getDataAs<AttributeMatrix>(parentPath) != nullptr)
  {
    const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(parentPath);
    tDims = attrMatrix.getShape();
  }
  else if(dataStructure.getDataAs<IGridGeometry>(parentPath) != nullptr)
  {
    const auto& geom = dataStructure.getDataRefAs<IGridGeometry>(parentPath);
    tDims = geom.getDimensions().toContainer<std::vector<usize>>();
    std::reverse(tDims.rbegin(), tDims.rend());
  }

  complex::Result<OutputActions> resultOutputActions;
  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint32, tDims, std::vector<usize>{6}, pFeatureRectArrayPathValue);
  resultOutputActions.value().actions.push_back(std::move(createArrayAction));

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureRectFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  ComputeFeatureRectInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureRectArrayPath = filterArgs.value<DataPath>(k_FeatureRectArrayPath_Key);

  DataPath parentPath = inputValues.FeatureRectArrayPath.getParent();
  if(!parentPath.empty())
  {
    inputValues.ParentDataObjectName = parentPath.getTargetName();
  }

  return ComputeFeatureRect(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
