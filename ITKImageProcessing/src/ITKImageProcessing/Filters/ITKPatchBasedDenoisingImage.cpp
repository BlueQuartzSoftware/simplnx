#include "ITKPatchBasedDenoisingImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKPatchBasedDenoisingImage::name() const
{
  return FilterTraits<ITKPatchBasedDenoisingImage>::name.str();
}

std::string ITKPatchBasedDenoisingImage::className() const
{
  return FilterTraits<ITKPatchBasedDenoisingImage>::className;
}

Uuid ITKPatchBasedDenoisingImage::uuid() const
{
  return FilterTraits<ITKPatchBasedDenoisingImage>::uuid;
}

std::string ITKPatchBasedDenoisingImage::humanName() const
{
  return "ITK::Patch Based Denoising Image Filter";
}

Parameters ITKPatchBasedDenoisingImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_NoiseModel_Key, "Noise Model", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Float64Parameter>(k_KernelBandwidthSigma_Key, "KernelBandwidthSigma", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_PatchRadius_Key, "PatchRadius", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfIterations_Key, "NumberOfIterations", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfSamplePatches_Key, "NumberOfSamplePatches", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_SampleVariance_Key, "SampleVariance", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NoiseSigma_Key, "NoiseSigma", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NoiseModelFidelityWeight_Key, "NoiseModelFidelityWeight", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_AlwaysTreatComponentsAsEuclidean_Key, "AlwaysTreatComponentsAsEuclidean", "", false));
  params.insert(std::make_unique<BoolParameter>(k_KernelBandwidthEstimation_Key, "KernelBandwidthEstimation", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_KernelBandwidthMultiplicationFactor_Key, "KernelBandwidthMultiplicationFactor", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_KernelBandwidthUpdateFrequency_Key, "KernelBandwidthUpdateFrequency", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_KernelBandwidthFractionPixelsForEstimation_Key, "KernelBandwidthFractionPixelsForEstimation", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKPatchBasedDenoisingImage::clone() const
{
  return std::make_unique<ITKPatchBasedDenoisingImage>();
}

Result<OutputActions> ITKPatchBasedDenoisingImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pNoiseModelValue = filterArgs.value<ChoicesParameter::ValueType>(k_NoiseModel_Key);
  auto pKernelBandwidthSigmaValue = filterArgs.value<float64>(k_KernelBandwidthSigma_Key);
  auto pPatchRadiusValue = filterArgs.value<float64>(k_PatchRadius_Key);
  auto pNumberOfIterationsValue = filterArgs.value<float64>(k_NumberOfIterations_Key);
  auto pNumberOfSamplePatchesValue = filterArgs.value<float64>(k_NumberOfSamplePatches_Key);
  auto pSampleVarianceValue = filterArgs.value<float64>(k_SampleVariance_Key);
  auto pNoiseSigmaValue = filterArgs.value<float64>(k_NoiseSigma_Key);
  auto pNoiseModelFidelityWeightValue = filterArgs.value<float64>(k_NoiseModelFidelityWeight_Key);
  auto pAlwaysTreatComponentsAsEuclideanValue = filterArgs.value<bool>(k_AlwaysTreatComponentsAsEuclidean_Key);
  auto pKernelBandwidthEstimationValue = filterArgs.value<bool>(k_KernelBandwidthEstimation_Key);
  auto pKernelBandwidthMultiplicationFactorValue = filterArgs.value<float64>(k_KernelBandwidthMultiplicationFactor_Key);
  auto pKernelBandwidthUpdateFrequencyValue = filterArgs.value<float64>(k_KernelBandwidthUpdateFrequency_Key);
  auto pKernelBandwidthFractionPixelsForEstimationValue = filterArgs.value<float64>(k_KernelBandwidthFractionPixelsForEstimation_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKPatchBasedDenoisingImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKPatchBasedDenoisingImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNoiseModelValue = filterArgs.value<ChoicesParameter::ValueType>(k_NoiseModel_Key);
  auto pKernelBandwidthSigmaValue = filterArgs.value<float64>(k_KernelBandwidthSigma_Key);
  auto pPatchRadiusValue = filterArgs.value<float64>(k_PatchRadius_Key);
  auto pNumberOfIterationsValue = filterArgs.value<float64>(k_NumberOfIterations_Key);
  auto pNumberOfSamplePatchesValue = filterArgs.value<float64>(k_NumberOfSamplePatches_Key);
  auto pSampleVarianceValue = filterArgs.value<float64>(k_SampleVariance_Key);
  auto pNoiseSigmaValue = filterArgs.value<float64>(k_NoiseSigma_Key);
  auto pNoiseModelFidelityWeightValue = filterArgs.value<float64>(k_NoiseModelFidelityWeight_Key);
  auto pAlwaysTreatComponentsAsEuclideanValue = filterArgs.value<bool>(k_AlwaysTreatComponentsAsEuclidean_Key);
  auto pKernelBandwidthEstimationValue = filterArgs.value<bool>(k_KernelBandwidthEstimation_Key);
  auto pKernelBandwidthMultiplicationFactorValue = filterArgs.value<float64>(k_KernelBandwidthMultiplicationFactor_Key);
  auto pKernelBandwidthUpdateFrequencyValue = filterArgs.value<float64>(k_KernelBandwidthUpdateFrequency_Key);
  auto pKernelBandwidthFractionPixelsForEstimationValue = filterArgs.value<float64>(k_KernelBandwidthFractionPixelsForEstimation_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
