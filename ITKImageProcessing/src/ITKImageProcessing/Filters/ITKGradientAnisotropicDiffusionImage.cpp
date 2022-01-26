#include "ITKGradientAnisotropicDiffusionImage.hpp"

/**
 * This filter only works with certain kinds of data. We
 * enable the types that the filter will compile against. The
 * Allowed PixelTypes as defined in SimpleITK are:
 *   RealPixelIDTypeList
 */
#define ITK_REAL_PIXEL_ID_TYPE_LIST 1
#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1
#define ITK_ARRAY_HELPER_NAMESPACE GradientAnisotropicDiffusionImage

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkGradientAnisotropicDiffusionImageFilter.h>

using namespace complex;

namespace
{
struct ITKGradientAnisotropicDiffusionImageCreationFunctor
{
  float64 pTimeStep = 0.125;
  double pConductanceParameter = 3;
  unsigned int pConductanceScalingUpdateInterval = 1u;
  uint32_t pNumberOfIterations = 5u;

  template <class InputImageType, class OutputImageType, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::GradientAnisotropicDiffusionImageFilter<InputImageType, OutputImageType>;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetTimeStep(pTimeStep);
    filter->SetConductanceParameter(pConductanceParameter);
    filter->SetConductanceScalingUpdateInterval(pConductanceScalingUpdateInterval);
    filter->SetNumberOfIterations(pNumberOfIterations);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKGradientAnisotropicDiffusionImage::name() const
{
  return FilterTraits<ITKGradientAnisotropicDiffusionImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKGradientAnisotropicDiffusionImage::className() const
{
  return FilterTraits<ITKGradientAnisotropicDiffusionImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKGradientAnisotropicDiffusionImage::uuid() const
{
  return FilterTraits<ITKGradientAnisotropicDiffusionImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKGradientAnisotropicDiffusionImage::humanName() const
{
  return "ITK::GradientAnisotropicDiffusionImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKGradientAnisotropicDiffusionImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKGradientAnisotropicDiffusionImage", "ITKAnisotropicSmoothing", "AnisotropicSmoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKGradientAnisotropicDiffusionImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_TimeStep_Key, "TimeStep", "", 0.125));
  params.insert(std::make_unique<Float64Parameter>(k_ConductanceParameter_Key, "ConductanceParameter", "", 3));
  params.insert(std::make_unique<UInt32Parameter>(k_ConductanceScalingUpdateInterval_Key, "ConductanceScalingUpdateInterval", "", 1u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfIterations_Key, "NumberOfIterations", "", 5u));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKGradientAnisotropicDiffusionImage::clone() const
{
  return std::make_unique<ITKGradientAnisotropicDiffusionImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKGradientAnisotropicDiffusionImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pTimeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto pConductanceParameter = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto pConductanceScalingUpdateInterval = filterArgs.value<unsigned int>(k_ConductanceScalingUpdateInterval_Key);
  auto pNumberOfIterations = filterArgs.value<uint32_t>(k_NumberOfIterations_Key);

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
Result<> ITKGradientAnisotropicDiffusionImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto pTimeStep = filterArgs.value<float64>(k_TimeStep_Key);
  auto pConductanceParameter = filterArgs.value<float64>(k_ConductanceParameter_Key);
  auto pConductanceScalingUpdateInterval = filterArgs.value<unsigned int>(k_ConductanceScalingUpdateInterval_Key);
  auto pNumberOfIterations = filterArgs.value<uint32_t>(k_NumberOfIterations_Key);

  /****************************************************************************
   * Create the functor object that will instantiate the correct itk filter
   ***************************************************************************/
  ::ITKGradientAnisotropicDiffusionImageCreationFunctor itkFunctor = {pTimeStep, pConductanceParameter, pConductanceScalingUpdateInterval, pNumberOfIterations};

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
