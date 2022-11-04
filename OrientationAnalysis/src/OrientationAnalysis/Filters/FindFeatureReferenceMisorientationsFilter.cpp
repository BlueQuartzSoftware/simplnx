#include "FindFeatureReferenceMisorientationsFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindFeatureReferenceMisorientations.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindFeatureReferenceMisorientationsFilter::name() const
{
  return FilterTraits<FindFeatureReferenceMisorientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceMisorientationsFilter::className() const
{
  return FilterTraits<FindFeatureReferenceMisorientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureReferenceMisorientationsFilter::uuid() const
{
  return FilterTraits<FindFeatureReferenceMisorientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureReferenceMisorientationsFilter::humanName() const
{
  return "Find Feature Reference Misorientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureReferenceMisorientationsFilter::defaultTags() const
{
  return {"#Statistics", "#Crystallography", "#Misorientation"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureReferenceMisorientationsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_ReferenceOrientation_Key, "Reference Orientation", "", 0, ChoicesParameter::Choices{"Average Orientation", "Orientation at Feature Centroid"}));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath({"CellData", "FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "", DataPath({"CellData", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath({"CellData", "Quats"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GBEuclideanDistancesArrayPath_Key, "Boundary Euclidean Distances", "", DataPath({"CellData", "GBEuclideanDistances"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath({"CellFeatureData", "AvgQuats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Cell Feature Attribute Matrix", "", DataPath({"CellFeatureData"})));
  params.insertSeparator(Parameters::Separator{"Required Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"Ensemble Data", "CrystalStructures"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureReferenceMisorientationsArrayName_Key, "Feature Reference Misorientations", "", "FeatureReferenceMisorientations"));
  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureAvgMisorientationsArrayName_Key, "Average Misorientations", "", "FeatureAvgMisorientations"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ReferenceOrientation_Key, k_GBEuclideanDistancesArrayPath_Key, static_cast<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_ReferenceOrientation_Key, k_CellFeatureAttributeMatrixPath_Key, static_cast<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_ReferenceOrientation_Key, k_AvgQuatsArrayPath_Key, static_cast<ChoicesParameter::ValueType>(0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureReferenceMisorientationsFilter::clone() const
{
  return std::make_unique<FindFeatureReferenceMisorientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureReferenceMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                  const std::atomic_bool& shouldCancel) const
{
  auto pReferenceOrientationValue = filterArgs.value<ChoicesParameter::ValueType>(k_ReferenceOrientation_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pGBEuclideanDistancesArrayPathValue = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellFeatAttributeMatrixArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pFeatureReferenceMisorientationsArrayPathValue = pFeatureIdsArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_FeatureReferenceMisorientationsArrayName_Key));
  auto pFeatureAvgMisorientationsArrayNameValue = filterArgs.value<std::string>(k_FeatureAvgMisorientationsArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  DataPath cellDataGroup = pCellPhasesArrayPathValue.getParent();

  const auto& cellPhases = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);

  // Create output Feature Reference Misorientations
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, cellPhases.getIDataStore()->getTupleShape(), std::vector<usize>{1}, pFeatureReferenceMisorientationsArrayPathValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Create output Feature Average Misorientations
  {
    DataPath featAvgMisorientationsPath;
    std::vector<usize> tupleShape;
    if(pReferenceOrientationValue == 0)
    {
      auto cellFeatPath = pAvgQuatsArrayPathValue.getParent();
      featAvgMisorientationsPath = cellFeatPath.createChildPath(pFeatureAvgMisorientationsArrayNameValue);
      const auto& featureAvgQuats = dataStructure.getDataRefAs<Float32Array>(pAvgQuatsArrayPathValue);
      tupleShape = featureAvgQuats.getIDataStore()->getTupleShape();
    }
    else
    {
      auto* cellFeatAM = dataStructure.getDataAs<AttributeMatrix>(pCellFeatAttributeMatrixArrayPathValue);
      if(cellFeatAM == nullptr)
      {
        return {MakeErrorResult<OutputActions>(-94520, fmt::format("Could not find selected cell feature Attribute Matrix at path '{}'", pCellFeatAttributeMatrixArrayPathValue.toString()))};
      }
      featAvgMisorientationsPath = pCellFeatAttributeMatrixArrayPathValue.createChildPath(pFeatureAvgMisorientationsArrayNameValue);
      tupleShape = cellFeatAM->getShape();
    }

    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, featAvgMisorientationsPath);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureReferenceMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  FindFeatureReferenceMisorientationsInputValues inputValues;

  inputValues.ReferenceOrientation = filterArgs.value<ChoicesParameter::ValueType>(k_ReferenceOrientation_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.GBEuclideanDistancesArrayPath = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.FeatureReferenceMisorientationsArrayName = inputValues.FeatureIdsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_FeatureReferenceMisorientationsArrayName_Key));
  auto pCellFeatAttributeMatrixArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto featAvgMisorientationName = filterArgs.value<std::string>(k_FeatureAvgMisorientationsArrayName_Key);
  inputValues.FeatureAvgMisorientationsArrayName = inputValues.ReferenceOrientation == 0 ? inputValues.AvgQuatsArrayPath.getParent().createChildPath(featAvgMisorientationName) :
                                                                                           pCellFeatAttributeMatrixArrayPathValue.createChildPath(featAvgMisorientationName);

  return FindFeatureReferenceMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
