#include "ITKPatchBasedDenoisingImage.hpp"

/**
 * This filter only works with certain kinds of data. We
 * enable the types that the filter will compile against. The
 * Allowed PixelTypes as defined in SimpleITK are:
 *   BasicPixelIDTypeList
 */
#define ITK_BASIC_PIXEL_ID_TYPE_LIST 1
#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1
#define ITK_ARRAY_HELPER_NAMESPACE PatchBasedDenoisingImage

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#error NoiseModel = filterArgs.value<typename FilterType::NoiseModelType>(k_NoiseModel_Key);
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkPatchBasedDenoisingImageFilter.h>

using namespace complex;

namespace
{
struct ITKPatchBasedDenoisingImageCreationFunctor
{
  float64 pKernelBandwidthSigma = 400.0;
  uint32_t pPatchRadius = 4u;
  uint32_t pNumberOfIterations = 1u;
  uint32_t pNumberOfSamplePatches = 200u;
  float64 pSampleVariance = 400.0;
#error typename FilterType::NoiseModelType pNoiseModel;
  float64 pNoiseSigma = 0.0;
  float64 pNoiseModelFidelityWeight = 0.0;
  bool pAlwaysTreatComponentsAsEuclidean = false;
  bool pKernelBandwidthEstimation = false;
  float64 pKernelBandwidthMultiplicationFactor = 1.0;
  uint32_t pKernelBandwidthUpdateFrequency = 3u;
  float64 pKernelBandwidthFractionPixelsForEstimation = 0.2;

  template <class InputImageType, class OutputImageType, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::PatchBasedDenoisingImageFilter<InputImageType, OutputImageType>;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetKernelBandwidthSigma(pKernelBandwidthSigma);
    filter->SetPatchRadius(pPatchRadius);
    filter->SetNumberOfIterations(pNumberOfIterations);
    filter->SetNumberOfSamplePatches(pNumberOfSamplePatches);
    filter->SetSampleVariance(pSampleVariance);
    filter->SetNoiseModel(pNoiseModel);
    filter->SetNoiseSigma(pNoiseSigma);
    filter->SetNoiseModelFidelityWeight(pNoiseModelFidelityWeight);
    filter->SetAlwaysTreatComponentsAsEuclidean(pAlwaysTreatComponentsAsEuclidean);
    filter->SetKernelBandwidthEstimation(pKernelBandwidthEstimation);
    filter->SetKernelBandwidthMultiplicationFactor(pKernelBandwidthMultiplicationFactor);
    filter->SetKernelBandwidthUpdateFrequency(pKernelBandwidthUpdateFrequency);
    filter->SetKernelBandwidthFractionPixelsForEstimation(pKernelBandwidthFractionPixelsForEstimation);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKPatchBasedDenoisingImage::name() const
{
  return FilterTraits<ITKPatchBasedDenoisingImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKPatchBasedDenoisingImage::className() const
{
  return FilterTraits<ITKPatchBasedDenoisingImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKPatchBasedDenoisingImage::uuid() const
{
  return FilterTraits<ITKPatchBasedDenoisingImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKPatchBasedDenoisingImage::humanName() const
{
  return "ITK::PatchBasedDenoisingImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKPatchBasedDenoisingImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKPatchBasedDenoisingImage", "ITKDenoising", "Denoising"};
}

//------------------------------------------------------------------------------
Parameters ITKPatchBasedDenoisingImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_KernelBandwidthSigma_Key, "KernelBandwidthSigma", "", 400.0));
  params.insert(std::make_unique<UInt32Parameter>(k_PatchRadius_Key, "PatchRadius", "", 4u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "NumberOfIterations", "", 1u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfSamplePatches_Key, "NumberOfSamplePatches", "", 200u));
  params.insert(std::make_unique<Float64Parameter>(k_SampleVariance_Key, "SampleVariance", "", 400.0));
  // NoiseModel: typename FilterType::NoiseModelType = "itk::simple::PatchBasedDenoisingImageFilter::NOMODEL"
  params.insert(std::make_unique<Float64Parameter>(k_NoiseSigma_Key, "NoiseSigma", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_NoiseModelFidelityWeight_Key, "NoiseModelFidelityWeight", "", 0.0));
  params.insert(std::make_unique<BoolParameter>(k_AlwaysTreatComponentsAsEuclidean_Key, "AlwaysTreatComponentsAsEuclidean", "", false));
  params.insert(std::make_unique<BoolParameter>(k_KernelBandwidthEstimation_Key, "KernelBandwidthEstimation", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_KernelBandwidthMultiplicationFactor_Key, "KernelBandwidthMultiplicationFactor", "", 1.0));
  params.insert(std::make_unique<UInt32Parameter>(k_KernelBandwidthUpdateFrequency_Key, "KernelBandwidthUpdateFrequency", "", 3u));
  params.insert(std::make_unique<Float64Parameter>(k_KernelBandwidthFractionPixelsForEstimation_Key, "KernelBandwidthFractionPixelsForEstimation", "", 0.2));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKPatchBasedDenoisingImage::clone() const
{
  return std::make_unique<ITKPatchBasedDenoisingImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKPatchBasedDenoisingImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto pKernelBandwidthSigma = filterArgs.value<float64>(k_KernelBandwidthSigma_Key);
  auto pPatchRadius = filterArgs.value<uint32_t>(k_PatchRadius_Key);
  auto pNumberOfIterations = filterArgs.value<uint32_t>(k_NumberOfIterations_Key);
  auto pNumberOfSamplePatches = filterArgs.value<uint32_t>(k_NumberOfSamplePatches_Key);
  auto pSampleVariance = filterArgs.value<float64>(k_SampleVariance_Key);
#error NoiseModel = filterArgs.value<typename FilterType::NoiseModelType>(k_NoiseModel_Key);
  auto pNoiseSigma = filterArgs.value<float64>(k_NoiseSigma_Key);
  auto pNoiseModelFidelityWeight = filterArgs.value<float64>(k_NoiseModelFidelityWeight_Key);
  auto pAlwaysTreatComponentsAsEuclidean = filterArgs.value<bool>(k_AlwaysTreatComponentsAsEuclidean_Key);
  auto pKernelBandwidthEstimation = filterArgs.value<bool>(k_KernelBandwidthEstimation_Key);
  auto pKernelBandwidthMultiplicationFactor = filterArgs.value<float64>(k_KernelBandwidthMultiplicationFactor_Key);
  auto pKernelBandwidthUpdateFrequency = filterArgs.value<uint32_t>(k_KernelBandwidthUpdateFrequency_Key);
  auto pKernelBandwidthFractionPixelsForEstimation = filterArgs.value<float64>(k_KernelBandwidthFractionPixelsForEstimation_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;
  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions = ITK::DataCheck(dataStructure, pSelectedInputArray, pImageGeomPath, pOutputArrayPath);

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

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

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKPatchBasedDenoisingImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto pKernelBandwidthSigma = filterArgs.value<float64>(k_KernelBandwidthSigma_Key);
  auto pPatchRadius = filterArgs.value<uint32_t>(k_PatchRadius_Key);
  auto pNumberOfIterations = filterArgs.value<uint32_t>(k_NumberOfIterations_Key);
  auto pNumberOfSamplePatches = filterArgs.value<uint32_t>(k_NumberOfSamplePatches_Key);
  auto pSampleVariance = filterArgs.value<float64>(k_SampleVariance_Key);
#error NoiseModel = filterArgs.value<typename FilterType::NoiseModelType>(k_NoiseModel_Key);
  auto pNoiseSigma = filterArgs.value<float64>(k_NoiseSigma_Key);
  auto pNoiseModelFidelityWeight = filterArgs.value<float64>(k_NoiseModelFidelityWeight_Key);
  auto pAlwaysTreatComponentsAsEuclidean = filterArgs.value<bool>(k_AlwaysTreatComponentsAsEuclidean_Key);
  auto pKernelBandwidthEstimation = filterArgs.value<bool>(k_KernelBandwidthEstimation_Key);
  auto pKernelBandwidthMultiplicationFactor = filterArgs.value<float64>(k_KernelBandwidthMultiplicationFactor_Key);
  auto pKernelBandwidthUpdateFrequency = filterArgs.value<uint32_t>(k_KernelBandwidthUpdateFrequency_Key);
  auto pKernelBandwidthFractionPixelsForEstimation = filterArgs.value<float64>(k_KernelBandwidthFractionPixelsForEstimation_Key);

  /****************************************************************************
   * Create the functor object that will instantiate the correct itk filter
   ***************************************************************************/
  ::ITKPatchBasedDenoisingImageCreationFunctor itkFunctor = {pKernelBandwidthSigma,
                                                             pPatchRadius,
                                                             pNumberOfIterations,
                                                             pNumberOfSamplePatches,
                                                             pSampleVariance,
                                                             pNoiseModel,
                                                             pNoiseSigma,
                                                             pNoiseModelFidelityWeight,
                                                             pAlwaysTreatComponentsAsEuclidean,
                                                             pKernelBandwidthEstimation,
                                                             pKernelBandwidthMultiplicationFactor,
                                                             pKernelBandwidthUpdateFrequency,
                                                             pKernelBandwidthFractionPixelsForEstimation};

  /****************************************************************************
   * Associate the output image with the Image Geometry for Visualization
   ***************************************************************************/
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(pOutputArrayPath);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  return ITK::Execute(dataStructure, pSelectedInputArray, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
