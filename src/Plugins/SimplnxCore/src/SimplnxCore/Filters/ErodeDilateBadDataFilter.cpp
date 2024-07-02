#include "ErodeDilateBadDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ErodeDilateBadData.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace nx::core
{

//------------------------------------------------------------------------------
std::string ErodeDilateBadDataFilter::name() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateBadDataFilter::className() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateBadDataFilter::uuid() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateBadDataFilter::humanName() const
{
  return "Erode/Dilate Bad Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateBadDataFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateBadDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<ChoicesParameter>(k_Operation_Key, "Operation", "Whether to dilate or erode", 0ULL, detail::k_OperationChoices));
  params.insert(std::make_unique<Int32Parameter>(k_NumIterations_Key, "Number of Iterations", "The number of iterations to use for erosion/dilation", 2));
  params.insert(std::make_unique<BoolParameter>(k_XDirOn_Key, "X Direction", "Whether to erode/dilate in the X direction", true));
  params.insert(std::make_unique<BoolParameter>(k_YDirOn_Key, "Y Direction", "Whether to erode/dilate in the Y direction", true));
  params.insert(std::make_unique<BoolParameter>(k_ZDirOn_Key, "Z Direction", "Whether to erode/dilate in the Z direction", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when performing the algorithm",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateBadDataFilter::clone() const
{
  return std::make_unique<ErodeDilateBadDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateBadDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pOperationValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);
  auto pXDirOnValue = filterArgs.value<bool>(k_XDirOn_Key);
  auto pYDirOnValue = filterArgs.value<bool>(k_YDirOn_Key);
  auto pZDirOnValue = filterArgs.value<bool>(k_ZDirOn_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  std::string featureModificationWarning = "By modifying the cell level data, any feature data that was previously computed will most likely be invalid at this point. Filters that compute feature "
                                           "level data should be rerun to ensure accurate final results from your pipeline.";
  preflightUpdatedValues.emplace_back(PreflightValue{"Feature Data Modification Warning", featureModificationWarning});
  resultOutputActions.warnings().push_back(Warning{-14600, featureModificationWarning});

  if(pOperationValue != detail::k_DilateIndex && pOperationValue != detail::k_ErodeIndex)
  {
    MakeErrorResult(-16700, fmt::format("Operation Selection must be 0 (Dilate) or 1 (Erode). {} was passed into the filter. ", pOperationValue));
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pFeatureIdsArrayPathValue.getParent(), pIgnoredDataArrayPathsValue);

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateBadDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  ErodeDilateBadDataInputValues inputValues;

  inputValues.Operation = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  inputValues.NumIterations = filterArgs.value<int32>(k_NumIterations_Key);
  inputValues.XDirOn = filterArgs.value<bool>(k_XDirOn_Key);
  inputValues.YDirOn = filterArgs.value<bool>(k_YDirOn_Key);
  inputValues.ZDirOn = filterArgs.value<bool>(k_ZDirOn_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ErodeDilateBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_DirectionKey = "Direction";
constexpr StringLiteral k_NumIterationsKey = "NumIterations";
constexpr StringLiteral k_XDirOnKey = "XDirOn";
constexpr StringLiteral k_YDirOnKey = "YDirOn";
constexpr StringLiteral k_ZDirOnKey = "ZDirOn";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> ErodeDilateBadDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ErodeDilateBadDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DirectionKey, k_Operation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumIterationsKey, k_NumIterations_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_XDirOnKey, k_XDirOn_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_YDirOnKey, k_YDirOn_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_ZDirOnKey, k_ZDirOn_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
