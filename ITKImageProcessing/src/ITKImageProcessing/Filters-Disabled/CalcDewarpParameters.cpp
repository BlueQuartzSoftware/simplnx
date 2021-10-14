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
std::string CalcDewarpParameters::name() const
{
  return FilterTraits<CalcDewarpParameters>::name.str();
}

std::string CalcDewarpParameters::className() const
{
  return FilterTraits<CalcDewarpParameters>::className;
}

Uuid CalcDewarpParameters::uuid() const
{
  return FilterTraits<CalcDewarpParameters>::uuid;
}

std::string CalcDewarpParameters::humanName() const
{
  return "Calculate Dewarp Parameters";
}

Parameters CalcDewarpParameters::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<MontageStructureSelectionFilterParameter>(k_MontageName_Key, "Montage Name", "", {}));
  params.insertSeparator(Parameters::Separator{"Amoeba Optimizer"});
  params.insert(std::make_unique<Int32Parameter>(k_MaxIterations_Key, "Max Iterations", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_Delta_Key, "Delta", "", 1234356));
  params.insert(std::make_unique<Float64Parameter>(k_FractionalTolerance_Key, "Fractional Convergence Tolerance", "", 2.3456789));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SpecifyInitialSimplex_Key, "Specify Initial Simplex", "", false));
  params.insert(std::make_unique<EbsdWarpPolynomialFilterParameter>(k_XFactors_Key, "X Factors", "", {}));
  params.insert(std::make_unique<EbsdWarpPolynomialFilterParameter>(k_YFactors_Key, "Y Factors", "", {}));
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

IFilter::UniquePointer CalcDewarpParameters::clone() const
{
  return std::make_unique<CalcDewarpParameters>();
}

Result<OutputActions> CalcDewarpParameters::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
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

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CalcDewarpParametersAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CalcDewarpParameters::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
