#include "ComputeFeatureRectFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureRect.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
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
  return {className(), "#Statistics", "#Reconstruction", "#Rect", "#Calculate"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureRectFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{{"FeatureIds"}},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureDataAttributeMatrixPath_Key, "Feature Data Attribute Matrix",
                                                              "The path to the feature data attribute matrix associated with the input feature ids array", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));
  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureRectArrayPath_Key, "Feature Rect", "The feature rect calculated from the feature ids", "FeatureRect"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureRectFilter::clone() const
{
  return std::make_unique<ComputeFeatureRectFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureRectFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureDataAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureDataAttributeMatrixPath_Key);
  auto pFeatureRectArrayNameValue = filterArgs.value<std::string>(k_FeatureRectArrayPath_Key);

  DataPath featureRectArrayPath = pFeatureDataAttributeMatrixPathValue.createChildPath(pFeatureRectArrayNameValue);

  const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pFeatureDataAttributeMatrixPathValue);
  std::vector<usize> tDims = attrMatrix.getShape();

  /*
   * This output Feature Rect array assumes that the original dataset has dimensions that are no larger than uint32.
   * This may not be true in the future, and should probably be reviewed and updated later.
   */
  for(usize i = 0; i < tDims.size(); i++)
  {
    const auto& dim = tDims[i];
    if(dim > std::numeric_limits<uint32>::max())
    {
      return {MakeErrorResult<OutputActions>(
          -2000, fmt::format("Data Object at '{}': Dimension {} has a value of {}, which is larger than the maximum uint32 value ({}).  Please contact BlueQuartz Software for further information.",
                             featureRectArrayPath.toString(), i + 1, dim, std::numeric_limits<uint32>::max()))};
    }
  }

  nx::core::Result<OutputActions> resultOutputActions;
  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint32, tDims, std::vector<usize>{6}, featureRectArrayPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  std::vector<PreflightValue> preflightUpdatedValues;

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureRectFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  ComputeFeatureRectInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureDataAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureDataAttributeMatrixPath_Key);
  inputValues.FeatureRectArrayPath = inputValues.FeatureDataAttributeMatrixPath.createChildPath(filterArgs.value<std::string>(k_FeatureRectArrayPath_Key));

  return ComputeFeatureRect(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeatureRectArrayPathKey = "FeatureRectArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeatureRectFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureRectFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureRectArrayPathKey, k_FeatureDataAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_FeatureRectArrayPathKey, k_FeatureRectArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
