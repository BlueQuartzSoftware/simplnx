#include "PrincipalComponentAnalysis.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string PrincipalComponentAnalysis::name() const
{
  return FilterTraits<PrincipalComponentAnalysis>::name.str();
}

//------------------------------------------------------------------------------
std::string PrincipalComponentAnalysis::className() const
{
  return FilterTraits<PrincipalComponentAnalysis>::className;
}

//------------------------------------------------------------------------------
Uuid PrincipalComponentAnalysis::uuid() const
{
  return FilterTraits<PrincipalComponentAnalysis>::uuid;
}

//------------------------------------------------------------------------------
std::string PrincipalComponentAnalysis::humanName() const
{
  return "Principal Component Analysis";
}

//------------------------------------------------------------------------------
std::vector<std::string> PrincipalComponentAnalysis::defaultTags() const
{
  return {"#DREAM3DReview", "#Dimensionality Reduction"};
}

//------------------------------------------------------------------------------
Parameters PrincipalComponentAnalysis::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays for Computing Principal Components", "",
                                                               MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}, MultiArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ChoicesParameter>(k_MatrixApproach_Key, "Matrix Approach", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ProjectDataSpace_Key, "Project Data Space", "", false));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfDimensionsForProjection_Key, "Number of Dimensions for Projection", "", 1234356));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ProjectedDataSpaceArrayPath_Key, "Projected Data Space", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Principal Component Data"});
  params.insert(std::make_unique<StringParameter>(k_PCAttributeMatrixName_Key, "Principal Component Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_PCEigenvaluesName_Key, "Principal Component Eigenvalues", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_PCEigenvectorsName_Key, "Principal Component Eigenvectors", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ProjectDataSpace_Key, k_NumberOfDimensionsForProjection_Key, true);
  params.linkParameters(k_ProjectDataSpace_Key, k_ProjectedDataSpaceArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PrincipalComponentAnalysis::clone() const
{
  return std::make_unique<PrincipalComponentAnalysis>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PrincipalComponentAnalysis::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pMatrixApproachValue = filterArgs.value<ChoicesParameter::ValueType>(k_MatrixApproach_Key);
  auto pProjectDataSpaceValue = filterArgs.value<bool>(k_ProjectDataSpace_Key);
  auto pNumberOfDimensionsForProjectionValue = filterArgs.value<int32>(k_NumberOfDimensionsForProjection_Key);
  auto pProjectedDataSpaceArrayPathValue = filterArgs.value<DataPath>(k_ProjectedDataSpaceArrayPath_Key);
  auto pPCAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_PCAttributeMatrixName_Key);
  auto pPCEigenvaluesNameValue = filterArgs.value<StringParameter::ValueType>(k_PCEigenvaluesName_Key);
  auto pPCEigenvectorsNameValue = filterArgs.value<StringParameter::ValueType>(k_PCEigenvectorsName_Key);

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

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  // This block is commented out because it needs some variables to be filled in.
  {
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pProjectedDataSpaceArrayPathValue);
    // resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> PrincipalComponentAnalysis::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pMatrixApproachValue = filterArgs.value<ChoicesParameter::ValueType>(k_MatrixApproach_Key);
  auto pProjectDataSpaceValue = filterArgs.value<bool>(k_ProjectDataSpace_Key);
  auto pNumberOfDimensionsForProjectionValue = filterArgs.value<int32>(k_NumberOfDimensionsForProjection_Key);
  auto pProjectedDataSpaceArrayPathValue = filterArgs.value<DataPath>(k_ProjectedDataSpaceArrayPath_Key);
  auto pPCAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_PCAttributeMatrixName_Key);
  auto pPCEigenvaluesNameValue = filterArgs.value<StringParameter::ValueType>(k_PCEigenvaluesName_Key);
  auto pPCEigenvectorsNameValue = filterArgs.value<StringParameter::ValueType>(k_PCEigenvectorsName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
