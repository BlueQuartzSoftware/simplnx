#include "ComputeBoundaryStrengthsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeBoundaryStrengths.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeBoundaryStrengthsFilter::name() const
{
  return FilterTraits<ComputeBoundaryStrengthsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeBoundaryStrengthsFilter::className() const
{
  return FilterTraits<ComputeBoundaryStrengthsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeBoundaryStrengthsFilter::uuid() const
{
  return FilterTraits<ComputeBoundaryStrengthsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeBoundaryStrengthsFilter::humanName() const
{
  return "Compute Feature Boundary Strength Metrics";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeBoundaryStrengthsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeBoundaryStrengthsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Loading_Key, "Loading Direction (XYZ)", "The loading axis for the sample", std::vector<float64>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Input Triangle Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Data Array that specifies which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions",
                                                          "Data Array that specifies the average orientation of each Feature in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "Data Array that specifies to which Ensemble each Feature belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each phase", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshF1sArrayName_Key, "F1s", "DataArray Name to store the calculated F1s Values", "F1s"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshF1sptsArrayName_Key, "F1spts", "DataArray Name to store the calculated F1spts Values", "F1s points"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshF7sArrayName_Key, "F7s", "DataArray Name to store the calculated F7s Values", "F7s"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SurfaceMeshmPrimesArrayName_Key, "mPrimes", "DataArray Name to store the calculated mPrimes Values", "mPrimes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeBoundaryStrengthsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeBoundaryStrengthsFilter::clone() const
{
  return std::make_unique<ComputeBoundaryStrengthsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeBoundaryStrengthsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshF1sArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshF1sArrayName_Key);
  auto pSurfaceMeshF1sptsArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshF1sptsArrayName_Key);
  auto pSurfaceMeshF7sArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshF7sArrayName_Key);
  auto pSurfaceMeshmPrimesArrayNameValue = filterArgs.value<std::string>(k_SurfaceMeshmPrimesArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<usize> faceLabelsTupShape = dataStructure.getDataAs<Int32Array>(pSurfaceMeshFaceLabelsArrayPathValue)->getTupleShape();
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsTupShape, std::vector<usize>{2}, pSurfaceMeshFaceLabelsArrayPathValue.replaceName(pSurfaceMeshF1sArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsTupShape, std::vector<usize>{2}, pSurfaceMeshFaceLabelsArrayPathValue.replaceName(pSurfaceMeshF1sptsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsTupShape, std::vector<usize>{2}, pSurfaceMeshFaceLabelsArrayPathValue.replaceName(pSurfaceMeshF7sArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  {
    auto action =
        std::make_unique<CreateArrayAction>(DataType::float32, faceLabelsTupShape, std::vector<usize>{2}, pSurfaceMeshFaceLabelsArrayPathValue.replaceName(pSurfaceMeshmPrimesArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeBoundaryStrengthsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  ComputeBoundaryStrengthsInputValues inputValues;

  inputValues.Loading = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Loading_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  DataPath surfaceMeshParentPath = inputValues.SurfaceMeshFaceLabelsArrayPath.getParent();
  inputValues.SurfaceMeshF1sArrayName = surfaceMeshParentPath.createChildPath(filterArgs.value<std::string>(k_SurfaceMeshF1sArrayName_Key));
  inputValues.SurfaceMeshF1sptsArrayName = surfaceMeshParentPath.createChildPath(filterArgs.value<std::string>(k_SurfaceMeshF1sptsArrayName_Key));
  inputValues.SurfaceMeshF7sArrayName = surfaceMeshParentPath.createChildPath(filterArgs.value<std::string>(k_SurfaceMeshF7sArrayName_Key));
  inputValues.SurfaceMeshmPrimesArrayName = surfaceMeshParentPath.createChildPath(filterArgs.value<std::string>(k_SurfaceMeshmPrimesArrayName_Key));

  return ComputeBoundaryStrengths(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_LoadingKey = "Loading";
constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteral k_AvgQuatsArrayPathKey = "AvgQuatsArrayPath";
constexpr StringLiteral k_FeaturePhasesArrayPathKey = "FeaturePhasesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_SurfaceMeshF1sArrayNameKey = "SurfaceMeshF1sArrayName";
constexpr StringLiteral k_SurfaceMeshF1sptsArrayNameKey = "SurfaceMeshF1sptsArrayName";
constexpr StringLiteral k_SurfaceMeshF7sArrayNameKey = "SurfaceMeshF7sArrayName";
constexpr StringLiteral k_SurfaceMeshmPrimesArrayNameKey = "SurfaceMeshmPrimesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeBoundaryStrengthsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeBoundaryStrengthsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleVec3FilterParameterConverter>(args, json, SIMPL::k_LoadingKey, k_Loading_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_AvgQuatsArrayPathKey, k_AvgQuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeaturePhasesArrayPathKey, k_FeaturePhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshF1sArrayNameKey, k_SurfaceMeshF1sArrayName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshF1sptsArrayNameKey, k_SurfaceMeshF1sptsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshF7sArrayNameKey, k_SurfaceMeshF7sArrayName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshmPrimesArrayNameKey, k_SurfaceMeshmPrimesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
