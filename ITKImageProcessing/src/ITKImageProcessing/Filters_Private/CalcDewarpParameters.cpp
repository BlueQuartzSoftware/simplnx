#include "CalcDewarpParameters.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/EbsdWarpPolynomialFilterParameter.hpp"
#include "complex/Parameters/MontageStructureSelectionFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CalcDewarpParameters::name() const
{
  return FilterTraits<CalcDewarpParameters>::name.str();
}

//------------------------------------------------------------------------------
std::string CalcDewarpParameters::className() const
{
  return FilterTraits<CalcDewarpParameters>::className;
}

//------------------------------------------------------------------------------
Uuid CalcDewarpParameters::uuid() const
{
  return FilterTraits<CalcDewarpParameters>::uuid;
}

//------------------------------------------------------------------------------
std::string CalcDewarpParameters::humanName() const
{
  return "Calculate Dewarp Parameters";
}

//------------------------------------------------------------------------------
std::vector<std::string> CalcDewarpParameters::defaultTags() const
{
  return {"#Processing", "#Processing"};
}

//------------------------------------------------------------------------------
Parameters CalcDewarpParameters::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<MontageStructureSelectionFilterParameter>(k_MontageName_Key, "Montage Name", "", {}));
  params.insertSeparator(Parameters::Separator{"Amoeba Optimizer"});
  params.insert(std::make_unique<Int32Parameter>(k_MaxIterations_Key, "Max Iterations", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_Delta_Key, "Delta", "", 1234356));
  params.insert(std::make_unique<Float64Parameter>(k_FractionalTolerance_Key, "Fractional Convergence Tolerance", "", 2.3456789));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SpecifyInitialSimplex_Key, "Specify Initial Simplex", "", false));
  /*[x]*/ params.insert(std::make_unique<EbsdWarpPolynomialFilterParameter>(k_XFactors_Key, "X Factors", "", {}));
  /*[x]*/ params.insert(std::make_unique<EbsdWarpPolynomialFilterParameter>(k_YFactors_Key, "Y Factors", "", {}));
  params.insertSeparator(Parameters::Separator{"Montage Data Path"});
  params.insert(std::make_unique<StringParameter>(k_AttributeMatrixName_Key, "Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_IPFColorsArrayName_Key, "IPF Colors Array Name", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Transformation Arrays"});
  params.insert(std::make_unique<StringParameter>(k_TransformDCName_Key, "Transform Container", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_TransformMatrixName_Key, "Transform Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_TransformArrayName_Key, "Transform Array", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SpecifyInitialSimplex_Key, k_XFactors_Key, true);
  params.linkParameters(k_SpecifyInitialSimplex_Key, k_YFactors_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CalcDewarpParameters::clone() const
{
  return std::make_unique<CalcDewarpParameters>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CalcDewarpParameters::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pMontageNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageName_Key);
  auto pMaxIterationsValue = filterArgs.value<int32>(k_MaxIterations_Key);
  auto pDeltaValue = filterArgs.value<int32>(k_Delta_Key);
  auto pFractionalToleranceValue = filterArgs.value<float64>(k_FractionalTolerance_Key);
  auto pSpecifyInitialSimplexValue = filterArgs.value<bool>(k_SpecifyInitialSimplex_Key);
  auto pXFactorsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_XFactors_Key);
  auto pYFactorsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_YFactors_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  auto pIPFColorsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_IPFColorsArrayName_Key);
  auto pTransformDCNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformDCName_Key);
  auto pTransformMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformMatrixName_Key);
  auto pTransformArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformArrayName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

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

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CalcDewarpParameters::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMontageNameValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageName_Key);
  auto pMaxIterationsValue = filterArgs.value<int32>(k_MaxIterations_Key);
  auto pDeltaValue = filterArgs.value<int32>(k_Delta_Key);
  auto pFractionalToleranceValue = filterArgs.value<float64>(k_FractionalTolerance_Key);
  auto pSpecifyInitialSimplexValue = filterArgs.value<bool>(k_SpecifyInitialSimplex_Key);
  auto pXFactorsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_XFactors_Key);
  auto pYFactorsValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_YFactors_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  auto pIPFColorsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_IPFColorsArrayName_Key);
  auto pTransformDCNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformDCName_Key);
  auto pTransformMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformMatrixName_Key);
  auto pTransformArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_TransformArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
