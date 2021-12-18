#include "ITKFFTNormalizedCorrelationImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkFFTNormalizedCorrelationImageFilter.h>

namespace
{
struct ITKFFTNormalizedCorrelationImageFilterCreationFunctor
{
  float64 m_RequiredNumberOfOverlappingPixels;
  float64 m_RequiredFractionOfOverlappingPixels;
  DataPath m_SelectedCellArrayPath;
  DataPath m_MovingCellArrayPath;
  template <typename InputImageType, typename OutputImageType, unsigned int Dimension>
  auto operator()() const
  {
    typedef itk::Image<OutputPixelType, Dimension> IntermediateImageType;

    typedef itk::FFTNormalizedCorrelationImageFilter<InputImageType, IntermediateImageType> FilterType;
    typename FilterType::Pointer filter = FilterType::New();

    filter->SetRequiredNumberOfOverlappingPixels(static_cast<itk::SizeValueType>(m_RequiredNumberOfOverlappingPixels));
    filter->SetRequiredFractionOfOverlappingPixels(static_cast<double>(m_RequiredFractionOfOverlappingPixels));

    typedef itk::InPlaceDream3DDataToImageFilter<InputPixelType, Dimension> toITKType;
    DataArrayPath dapMoving = getMovingCellArrayPath();
    DataContainer::Pointer dcMoving = getDataContainerArray()->getDataContainer(dapMoving.getDataContainerName());
    typename toITKType::Pointer toITKMoving = toITKType::New();
    toITKMoving->SetInput(dcMoving);
    toITKMoving->SetInPlace(true);
    toITKMoving->SetAttributeMatrixArrayName(getMovingCellArrayPath().getAttributeMatrixName().toStdString());
    toITKMoving->SetDataArrayName(getMovingCellArrayPath().getDataArrayName().toStdString());
    filter->SetMovingImage(toITKMoving->GetOutput());

    try
    {
      DataArrayPath dap = getSelectedCellArrayPath();
      DataContainer::Pointer dc = getDataContainerArray()->getDataContainer(dap.getDataContainerName());

      typename toITKType::Pointer toITK = toITKType::New();
      toITK->SetInput(dc);
      toITK->SetInPlace(true);
      toITK->SetAttributeMatrixArrayName(getSelectedCellArrayPath().getAttributeMatrixName().toStdString());
      toITK->SetDataArrayName(getSelectedCellArrayPath().getDataArrayName().toStdString());

      itk::Dream3DFilterInterruption::Pointer interruption = itk::Dream3DFilterInterruption::New();
      interruption->SetFilter(this);

      filter->SetFixedImage(toITK->GetOutput());
      filter->AddObserver(itk::ProgressEvent(), interruption);

      typedef itk::CastImageFilter<IntermediateImageType, OutputImageType> CasterType;
      typename CasterType::Pointer caster = CasterType::New();
      caster->SetInput(filter->GetOutput());
      caster->Update();

      typename OutputImageType::Pointer image = OutputImageType::New();
      image = caster->GetOutput();
      image->DisconnectPipeline();
      std::string outputArrayName(getNewCellArrayName().toStdString());

      typedef itk::InPlaceImageToDream3DDataFilter<OutputPixelType, Dimension> toDream3DType;
      typename toDream3DType::Pointer toDream3DFilter = toDream3DType::New();
      toDream3DFilter->SetInput(image);
      toDream3DFilter->SetInPlace(true);
      toDream3DFilter->SetAttributeMatrixArrayName(getSelectedCellArrayPath().getAttributeMatrixName().toStdString());
      toDream3DFilter->SetDataArrayName(outputArrayName);
      toDream3DFilter->SetDataContainer(dc);
      toDream3DFilter->Update();
    } catch(itk::ExceptionObject& err)
    {
      QString errorMessage = "ITK exception was thrown while filtering input image: %1";
      break;
    }
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKFFTNormalizedCorrelationImage::name() const
{
  return FilterTraits<ITKFFTNormalizedCorrelationImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKFFTNormalizedCorrelationImage::className() const
{
  return FilterTraits<ITKFFTNormalizedCorrelationImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKFFTNormalizedCorrelationImage::uuid() const
{
  return FilterTraits<ITKFFTNormalizedCorrelationImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKFFTNormalizedCorrelationImage::humanName() const
{
  return "ITK::FFT Normalized Correlation Image";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKFFTNormalizedCorrelationImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Registration"};
}

//------------------------------------------------------------------------------
Parameters ITKFFTNormalizedCorrelationImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_RequiredNumberOfOverlappingPixels_Key, "RequiredNumberOfOverlappingPixels", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_RequiredFractionOfOverlappingPixels_Key, "RequiredFractionOfOverlappingPixels", "", 2.3456789));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Fixed Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MovingCellArrayPath_Key, "Moving Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Filtered Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKFFTNormalizedCorrelationImage::clone() const
{
  return std::make_unique<ITKFFTNormalizedCorrelationImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKFFTNormalizedCorrelationImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pRequiredNumberOfOverlappingPixels = filterArgs.value<float64>(k_RequiredNumberOfOverlappingPixels_Key);
  auto pRequiredFractionOfOverlappingPixels = filterArgs.value<float64>(k_RequiredFractionOfOverlappingPixels_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMovingCellArrayPath = filterArgs.value<DataPath>(k_MovingCellArrayPath_Key);
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
Result<> ITKFFTNormalizedCorrelationImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRequiredNumberOfOverlappingPixels = filterArgs.value<float64>(k_RequiredNumberOfOverlappingPixels_Key);
  auto pRequiredFractionOfOverlappingPixels = filterArgs.value<float64>(k_RequiredFractionOfOverlappingPixels_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMovingCellArrayPath = filterArgs.value<DataPath>(k_MovingCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKFFTNormalizedCorrelationImageFilterCreationFunctor itkFunctor;
  itkFunctor.m_RequiredNumberOfOverlappingPixels = pRequiredNumberOfOverlappingPixels;
  itkFunctor.m_RequiredFractionOfOverlappingPixels = pRequiredFractionOfOverlappingPixels;
  itkFunctor.m_SelectedCellArrayPath = pSelectedCellArrayPath;
  itkFunctor.m_MovingCellArrayPath = pMovingCellArrayPath;

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
