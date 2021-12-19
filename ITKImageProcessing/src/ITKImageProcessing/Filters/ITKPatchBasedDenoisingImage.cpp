#include "ITKPatchBasedDenoisingImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkPatchBasedDenoisingImageFilter.h>

namespace
{
struct ITKPatchBasedDenoisingImageFilterCreationFunctor
{
  ChoicesParameter::ValueType m_NoiseModel;
  float64 m_KernelBandwidthSigma;
  float64 m_PatchRadius;
  float64 m_NumberOfIterations;
  float64 m_NumberOfSamplePatches;
  float64 m_SampleVariance;
  float64 m_NoiseSigma;
  float64 m_NoiseModelFidelityWeight;
  bool m_AlwaysTreatComponentsAsEuclidean;
  bool m_KernelBandwidthEstimation;
  float64 m_KernelBandwidthMultiplicationFactor;
  float64 m_KernelBandwidthUpdateFrequency;
  float64 m_KernelBandwidthFractionPixelsForEstimation;
  template <typename InputImageType, typename OutputImageType, unsigned int Dimension>
  auto operator()() const
  {
    typedef itk::Image<OutputPixelType, Dimension> RealImageType;
    typedef itk::PatchBasedDenoisingImageFilter<RealImageType, RealImageType> FilterType;
    typename FilterType::Pointer filter = FilterType::New();
    typedef itk::InPlaceDream3DDataToImageFilter<InputPixelType, Dimension> toITKType;
    DataArrayPath dap = getSelectedCellArrayPath();
    DataContainer::Pointer dc = getDataContainerArray()->getDataContainer(dap.getDataContainerName());
    typename toITKType::Pointer toITK = toITKType::New();
    toITK->SetInput(dc);
    toITK->SetInPlace(true);
    toITK->SetAttributeMatrixArrayName(getSelectedCellArrayPath().getAttributeMatrixName().toStdString());
    toITK->SetDataArrayName(getSelectedCellArrayPath().getDataArrayName().toStdString());
    typename FilterType::RealArrayType a(toITK->GetOutput()->GetNumberOfComponentsPerPixel());
    a.Fill(m_KernelBandwidthSigma);
    filter->SetKernelBandwidthSigma(a);
    filter->SetPatchRadius(static_cast<uint32_t>(m_PatchRadius));
    filter->SetNumberOfIterations(static_cast<uint32_t>(m_NumberOfIterations));
    if(this->m_NoiseSigma != 0.0)
    {
      filter->SetNoiseSigma(this->m_NoiseSigma);
    }
    filter->SetNoiseModelFidelityWeight(static_cast<double>(m_NoiseModelFidelityWeight));
    filter->SetAlwaysTreatComponentsAsEuclidean(static_cast<bool>(m_AlwaysTreatComponentsAsEuclidean));
    filter->SetKernelBandwidthEstimation(static_cast<bool>(m_KernelBandwidthEstimation));
    filter->SetKernelBandwidthMultiplicationFactor(static_cast<double>(m_KernelBandwidthMultiplicationFactor));
    filter->SetKernelBandwidthUpdateFrequency(static_cast<uint32_t>(m_KernelBandwidthUpdateFrequency));
    filter->SetKernelBandwidthFractionPixelsForEstimation(static_cast<double>(m_KernelBandwidthFractionPixelsForEstimation));
#if ITK_VERSION_MAJOR >= 5
    filter->SetNumberOfWorkUnits(this->getNumberOfThreads());
#else
    filter->SetNumberOfThreads(this->getNumberOfThreads());
#endif
    typedef itk::Statistics::GaussianRandomSpatialNeighborSubsampler<typename FilterType::PatchSampleType, typename RealImageType::RegionType> SamplerType;
    typename SamplerType::Pointer sampler = SamplerType::New();
    sampler->SetVariance(m_SampleVariance);
    sampler->SetRadius(itk::Math::Floor<unsigned int>(std::sqrt(m_SampleVariance) * 2.5));
    sampler->SetNumberOfResultsRequested(m_NumberOfSamplePatches);
    filter->SetSampler(sampler);
    typedef typename itk::PatchBasedDenoisingBaseImageFilter<RealImageType, RealImageType>::NoiseModelType NoiseModelType;
    NoiseModelType noiseModel = static_cast<NoiseModelType>(m_NoiseModel);
    filter->SetNoiseModel(noiseModel);

    return filter; /*   this->ITKImageProcessingBase::filterCastToFloat<InputPixelType, OutputPixelType, Dimension, FilterType, RealImageType>(filter); */
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
  return "ITK::Patch Based Denoising Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKPatchBasedDenoisingImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Denoising"};
}

//------------------------------------------------------------------------------
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
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Filtered Array", "", DataPath{}));

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
  auto pNoiseModel = filterArgs.value<ChoicesParameter::ValueType>(k_NoiseModel_Key);
  auto pKernelBandwidthSigma = filterArgs.value<float64>(k_KernelBandwidthSigma_Key);
  auto pPatchRadius = filterArgs.value<float64>(k_PatchRadius_Key);
  auto pNumberOfIterations = filterArgs.value<float64>(k_NumberOfIterations_Key);
  auto pNumberOfSamplePatches = filterArgs.value<float64>(k_NumberOfSamplePatches_Key);
  auto pSampleVariance = filterArgs.value<float64>(k_SampleVariance_Key);
  auto pNoiseSigma = filterArgs.value<float64>(k_NoiseSigma_Key);
  auto pNoiseModelFidelityWeight = filterArgs.value<float64>(k_NoiseModelFidelityWeight_Key);
  auto pAlwaysTreatComponentsAsEuclidean = filterArgs.value<bool>(k_AlwaysTreatComponentsAsEuclidean_Key);
  auto pKernelBandwidthEstimation = filterArgs.value<bool>(k_KernelBandwidthEstimation_Key);
  auto pKernelBandwidthMultiplicationFactor = filterArgs.value<float64>(k_KernelBandwidthMultiplicationFactor_Key);
  auto pKernelBandwidthUpdateFrequency = filterArgs.value<float64>(k_KernelBandwidthUpdateFrequency_Key);
  auto pKernelBandwidthFractionPixelsForEstimation = filterArgs.value<float64>(k_KernelBandwidthFractionPixelsForEstimation_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

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
  complex::Result<OutputActions> resultOutputActions;

  resultOutputActions = ITK::DataCheck(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath);

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
Result<> ITKPatchBasedDenoisingImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNoiseModel = filterArgs.value<ChoicesParameter::ValueType>(k_NoiseModel_Key);
  auto pKernelBandwidthSigma = filterArgs.value<float64>(k_KernelBandwidthSigma_Key);
  auto pPatchRadius = filterArgs.value<float64>(k_PatchRadius_Key);
  auto pNumberOfIterations = filterArgs.value<float64>(k_NumberOfIterations_Key);
  auto pNumberOfSamplePatches = filterArgs.value<float64>(k_NumberOfSamplePatches_Key);
  auto pSampleVariance = filterArgs.value<float64>(k_SampleVariance_Key);
  auto pNoiseSigma = filterArgs.value<float64>(k_NoiseSigma_Key);
  auto pNoiseModelFidelityWeight = filterArgs.value<float64>(k_NoiseModelFidelityWeight_Key);
  auto pAlwaysTreatComponentsAsEuclidean = filterArgs.value<bool>(k_AlwaysTreatComponentsAsEuclidean_Key);
  auto pKernelBandwidthEstimation = filterArgs.value<bool>(k_KernelBandwidthEstimation_Key);
  auto pKernelBandwidthMultiplicationFactor = filterArgs.value<float64>(k_KernelBandwidthMultiplicationFactor_Key);
  auto pKernelBandwidthUpdateFrequency = filterArgs.value<float64>(k_KernelBandwidthUpdateFrequency_Key);
  auto pKernelBandwidthFractionPixelsForEstimation = filterArgs.value<float64>(k_KernelBandwidthFractionPixelsForEstimation_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKPatchBasedDenoisingImageFilterCreationFunctor itkFunctor;
  itkFunctor.m_NoiseModel = pNoiseModel;
  itkFunctor.m_KernelBandwidthSigma = pKernelBandwidthSigma;
  itkFunctor.m_PatchRadius = pPatchRadius;
  itkFunctor.m_NumberOfIterations = pNumberOfIterations;
  itkFunctor.m_NumberOfSamplePatches = pNumberOfSamplePatches;
  itkFunctor.m_SampleVariance = pSampleVariance;
  itkFunctor.m_NoiseSigma = pNoiseSigma;
  itkFunctor.m_NoiseModelFidelityWeight = pNoiseModelFidelityWeight;
  itkFunctor.m_AlwaysTreatComponentsAsEuclidean = pAlwaysTreatComponentsAsEuclidean;
  itkFunctor.m_KernelBandwidthEstimation = pKernelBandwidthEstimation;
  itkFunctor.m_KernelBandwidthMultiplicationFactor = pKernelBandwidthMultiplicationFactor;
  itkFunctor.m_KernelBandwidthUpdateFrequency = pKernelBandwidthUpdateFrequency;
  itkFunctor.m_KernelBandwidthFractionPixelsForEstimation = pKernelBandwidthFractionPixelsForEstimation;

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(pOutputArrayPath);

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
