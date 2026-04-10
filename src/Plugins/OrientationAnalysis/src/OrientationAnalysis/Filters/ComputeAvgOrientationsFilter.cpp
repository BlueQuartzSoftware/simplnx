#include "ComputeAvgOrientationsFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/ComputeAvgOrientations.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeAvgOrientationsFilter::name() const
{
  return FilterTraits<ComputeAvgOrientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeAvgOrientationsFilter::className() const
{
  return FilterTraits<ComputeAvgOrientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeAvgOrientationsFilter::uuid() const
{
  return FilterTraits<ComputeAvgOrientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeAvgOrientationsFilter::humanName() const
{
  return "Compute Feature Average Orientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeAvgOrientationsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeAvgOrientationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseRodriguesAverage_Key, "Compute Rodrigues Average", "The original algorithm.", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseVonMisesFisher_Key, "Compute von Mises-Fisher Average", "The von Mises Fisher average algorithm.", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseWatson_Key, "Compute Watson Average", "The Watson average algorithm.", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"Cell Data", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellQuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation",
                                                          DataPath({"Cell Data", "Quats"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Cell Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{nx::core::DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "The path to the cell feature attribute matrix",
                                                                    DataPath({"Cell Feature Data"})));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_RodriguesQuatsArrayName_Key, "Average Rodrigues Quaternions",
                                                          "The name of the array specifying the average orientation based on the average Rodrigues vector of the Feature in quaternion representation",
                                                          "AvgQuats"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_RodriguesAvgEulerArrayName_Key, "Average Rodrigues Euler Angles",
                                                          "The name of the array specifying the orientation based on the average Rodrigues vector of each Feature in Bunge convention (Z-X-Z)",
                                                          "AvgEulerAngles"));

  params.insert(std::make_unique<DataObjectNameParameter>(
      k_VonMisesFisherAvgQuatsArrayName_Key, "Average von Mises-Fisher Quaternions",
      "The name of the array specifying the average orientation based on the von Mises-Fisher sampling of each Feature in quaternion representation", "vMF Avg Quats"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VonMisesFisherAvgEulerArrayName_Key, "Average von Mises-Fisher Euler Angles",
                                                          "The name of the array specifying the average orientation based on the von Mises-Fisher sampling of each Feature in Bunge convention (Z-X-Z)",
                                                          "vMF Avg EulerAngles"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VonMisesFisherKappaArrayName_Key, "von Mises-Fisher Kappa Values",
                                                          "The name of the array specifying the kappa values from the von Mises-Fisher sampling of each Feature", "vMF Kappas"));

  params.insert(std::make_unique<DataObjectNameParameter>(k_WatsonAvgQuatsArrayName_Key, "Average Watson Quaternions",
                                                          "The name of the array specifying the average orientation based on the Watson sampling of each Feature in quaternion representation",
                                                          "Watson Avg Quats"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_WatsonAvgEulerArrayName_Key, "Average Watson Angles",
                                                          "The name of the array specifying the average orientation based on the Watson sampling of each Feature in Bunge convention (Z-X-Z)",
                                                          "Watson Avg EulerAngles"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_WatsonKappaArrayName_Key, "Watson Kappa Values",
                                                          "The name of the array specifying the kappa values from the Watson sampling of each Feature", "Watson Kappas"));

  params.linkParameters(k_UseRodriguesAverage_Key, k_RodriguesAvgEulerArrayName_Key, true);
  params.linkParameters(k_UseRodriguesAverage_Key, k_RodriguesQuatsArrayName_Key, true);

  params.linkParameters(k_UseVonMisesFisher_Key, k_VonMisesFisherAvgQuatsArrayName_Key, true);
  params.linkParameters(k_UseVonMisesFisher_Key, k_VonMisesFisherAvgEulerArrayName_Key, true);
  params.linkParameters(k_UseVonMisesFisher_Key, k_VonMisesFisherKappaArrayName_Key, true);

  params.linkParameters(k_UseWatson_Key, k_WatsonAvgQuatsArrayName_Key, true);
  params.linkParameters(k_UseWatson_Key, k_WatsonAvgEulerArrayName_Key, true);
  params.linkParameters(k_UseWatson_Key, k_WatsonKappaArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeAvgOrientationsFilter::parametersVersion() const
{
  return 2;
  // Version 2 adds the ability to compute the von Mises-Fisher average and the Watson sampling average
  // Version 2 also adds the option to NOT compute the Eulers/Quats from the original algorithm
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeAvgOrientationsFilter::clone() const
{
  return std::make_unique<ComputeAvgOrientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeAvgOrientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pCellFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCellQuatsArrayPathValue = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  auto pUseRodriguesAverage_Key = filterArgs.value<bool>(k_UseRodriguesAverage_Key);
  auto pAvgQuatsArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_RodriguesQuatsArrayName_Key));
  auto pAvgEulerAnglesArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_RodriguesAvgEulerArrayName_Key));

  auto pUseVonMisesFisher = filterArgs.value<bool>(k_UseVonMisesFisher_Key);
  auto pVMFQuatsArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_VonMisesFisherAvgQuatsArrayName_Key));
  auto pVMFEulerAnglesArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_VonMisesFisherAvgEulerArrayName_Key));
  auto pVMFKappaArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_VonMisesFisherKappaArrayName_Key));

  auto pUseWatson = filterArgs.value<bool>(k_UseWatson_Key);
  auto pWatsonQuatsArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_WatsonAvgQuatsArrayName_Key));
  auto pWatsonEulerAnglesArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_WatsonAvgEulerArrayName_Key));
  auto pWatsonKappaArrayPathValue = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_WatsonKappaArrayName_Key));

  Result<OutputActions> resultOutputActions;

  std::vector<DataPath> dataPaths;

  dataPaths.push_back(pCellQuatsArrayPathValue);
  dataPaths.push_back(pCellPhasesArrayPathValue);
  dataPaths.push_back(pCellFeatureIdsArrayPathValue);

  // Make sure all the arrays have the same number of Tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto& cellFeatAttMatrix = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAttributeMatrixPathValue);

  // Create output DataStructure Items
  auto tupleShape = cellFeatAttMatrix.getShape();
  if(pUseRodriguesAverage_Key)
  {
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{4}, pAvgQuatsArrayPathValue);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, pAvgEulerAnglesArrayPathValue);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  if(pUseVonMisesFisher)
  {
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{4}, pVMFQuatsArrayPathValue);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, pVMFEulerAnglesArrayPathValue);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pVMFKappaArrayPathValue);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  if(pUseWatson)
  {
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{4}, pWatsonQuatsArrayPathValue);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, pWatsonEulerAnglesArrayPathValue);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pWatsonKappaArrayPathValue);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {resultOutputActions};
}

//------------------------------------------------------------------------------
Result<> ComputeAvgOrientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeAvgOrientationsInputValues inputValues;

  inputValues.cellFeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.cellQuatsArrayPath = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  inputValues.crystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  inputValues.useRodriguesAverage = filterArgs.value<bool>(k_UseRodriguesAverage_Key);
  inputValues.avgQuatsArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_RodriguesQuatsArrayName_Key));
  inputValues.avgEulerAnglesArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_RodriguesAvgEulerArrayName_Key));

  inputValues.useVonMisesAverage = filterArgs.value<bool>(k_UseVonMisesFisher_Key);
  inputValues.VMFQuatsArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_VonMisesFisherAvgQuatsArrayName_Key));
  inputValues.VMFEulerAnglesArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_VonMisesFisherAvgEulerArrayName_Key));
  inputValues.VMFKappaArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_VonMisesFisherKappaArrayName_Key));

  inputValues.useWatsonAverage = filterArgs.value<bool>(k_UseWatson_Key);
  inputValues.WatsonQuatsArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_WatsonAvgQuatsArrayName_Key));
  inputValues.WatsonEulerAnglesArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_WatsonAvgEulerArrayName_Key));
  inputValues.WatsonKappaArrayPath = pCellFeatureAttributeMatrixPathValue.createChildPath(filterArgs.value<std::string>(k_WatsonKappaArrayName_Key));

  inputValues.RandomSeed = 43514;

  // Let the Algorithm instance do the work
  return ComputeAvgOrientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
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

Result<Arguments> ComputeAvgOrientationsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeAvgOrientationsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_CellQuatsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_RodriguesQuatsArrayName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_AvgEulerAnglesArrayPathKey, k_RodriguesAvgEulerArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
