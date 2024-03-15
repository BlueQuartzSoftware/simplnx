#include "FindAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/FindAvgOrientations.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace
{

using FeatureIdsArrayType = Int32Array;
using GoodVoxelsArrayType = BoolArray;

inline constexpr int32 k_IncorrectInputArray = -7000;
inline constexpr int32 k_MissingInputArray = -7001;
inline constexpr int32 k_MissingOrIncorrectGoodVoxelsArray = -7002;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string FindAvgOrientationsFilter::name() const
{
  return FilterTraits<FindAvgOrientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindAvgOrientationsFilter::className() const
{
  return FilterTraits<FindAvgOrientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindAvgOrientationsFilter::uuid() const
{
  return FilterTraits<FindAvgOrientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindAvgOrientationsFilter::humanName() const
{
  return "Find Feature Average Orientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindAvgOrientationsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindAvgOrientationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs.", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"CellData", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellQuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation",
                                                          DataPath({"CellData", "Quats"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"CellEnsembleData", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{nx::core::DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "The path to the cell feature attribute matrix",
                                                                    DataPath({"CellFeatureData"})));
  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_AvgQuatsArrayName_Key, "Average Quaternions",
                                                          "The name of the array specifying the average orientation of the Feature in quaternion representation", "AvgQuats"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AvgEulerAnglesArrayName_Key, "Average Euler Angles",
                                                          "The name of the array specifying the orientation of each Feature in Bunge convention (Z-X-Z)", "AvgEulerAngles"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindAvgOrientationsFilter::clone() const
{
  return std::make_unique<FindAvgOrientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindAvgOrientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto pCellFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellQuatsArrayPathValue = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pAvgQuatsArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_AvgQuatsArrayName_Key));
  auto pAvgEulerAnglesArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_AvgEulerAnglesArrayName_Key));

  // Validate the Crystal Structures array
  const UInt32Array& crystalStructures = dataStructure.getDataRefAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  if(crystalStructures.getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Crystal Structures Input Array must be a 1 component Int32 array"}})};
  }

  std::vector<DataPath> dataPaths;

  // Validate the Quats array
  const Float32Array& quats = dataStructure.getDataRefAs<Float32Array>(pCellQuatsArrayPathValue);
  if(quats.getNumberOfComponents() != 4)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Quaternion Input Array must be a 4 component Float32 array"}})};
  }
  dataPaths.push_back(pCellQuatsArrayPathValue);

  // Validate the Phases array
  const Int32Array& phases = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);
  if(phases.getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Phases Input Array must be a 1 component Int32 array"}})};
  }
  dataPaths.push_back(pCellPhasesArrayPathValue);

  // Validate the FeatureIds array
  const Int32Array& featureIds = dataStructure.getDataRefAs<Int32Array>(pCellFeatureIdsArrayPathValue);
  if(featureIds.getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "FeatureIds Input Array must be a 1 component Int32 array"}})};
  }
  dataPaths.push_back(pCellFeatureIdsArrayPathValue);

  // Make sure all the arrays have the same number of Tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto* cellFeatAttMatrix = dataStructure.getDataAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue);
  if(cellFeatAttMatrix == nullptr)
  {
    return {
        nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, fmt::format("Could not find selected Attribute matrix at path '{}'", pCellFeatureAttributeMatrixPathValue.toString())}})};
  }

  // Create output DataStructure Items
  auto tDims = cellFeatAttMatrix->getShape();
  auto createAvgQuatAction = std::make_unique<CreateArrayAction>(DataType::float32, tDims, std::vector<usize>{4}, pAvgQuatsArrayPathValue);
  auto createAvgEulerAction = std::make_unique<CreateArrayAction>(DataType::float32, tDims, std::vector<usize>{3}, pAvgEulerAnglesArrayPathValue);

  OutputActions actions;
  actions.appendAction(std::move(createAvgQuatAction));
  actions.appendAction(std::move(createAvgEulerAction));

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindAvgOrientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  FindAvgOrientationsInputValues inputValues;

  inputValues.cellFeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.cellQuatsArrayPath = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  inputValues.crystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.avgQuatsArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_AvgQuatsArrayName_Key));
  inputValues.avgEulerAnglesArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_AvgEulerAnglesArrayName_Key));

  // Let the Algorithm instance do the work
  return FindAvgOrientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteral k_AvgEulerAnglesArrayPathKey = "AvgEulerAnglesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> FindAvgOrientationsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FindAvgOrientationsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_CellQuatsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_AvgEulerAnglesArrayPathKey, k_AvgEulerAnglesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
