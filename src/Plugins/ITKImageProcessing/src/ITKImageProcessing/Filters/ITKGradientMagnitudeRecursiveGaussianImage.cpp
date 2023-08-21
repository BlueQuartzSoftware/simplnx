#include "ITKGradientMagnitudeRecursiveGaussianImage.hpp"

/**
 * This filter only works with certain kinds of data. We
 * enable the types that the filter will compile against. The
 * Allowed PixelTypes as defined in SimpleITK are:
 *   BasicPixelIDTypeList
 * The filter defines the following output pixel types:
 *   float
 */
#define ITK_BASIC_PIXEL_ID_TYPE_LIST 1
#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1
#define ITK_ARRAY_HELPER_NAMESPACE GradientMagnitudeRecursiveGaussianImage

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>

using namespace complex;

namespace
{
/**
 * This filter uses a fixed output type.
 */
using FilterOutputType = float32;

struct ITKGradientMagnitudeRecursiveGaussianImageCreationFunctor
{
  float64 pSigma = 1.0;
  bool pNormalizeAcrossScale = false;

  template <class InputImageType, class OutputImageType, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::GradientMagnitudeRecursiveGaussianImageFilter<InputImageType, OutputImageType>;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetSigma(pSigma);
    filter->SetNormalizeAcrossScale(pNormalizeAcrossScale);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeRecursiveGaussianImage::name() const
{
  return FilterTraits<ITKGradientMagnitudeRecursiveGaussianImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeRecursiveGaussianImage::className() const
{
  return FilterTraits<ITKGradientMagnitudeRecursiveGaussianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKGradientMagnitudeRecursiveGaussianImage::uuid() const
{
  return FilterTraits<ITKGradientMagnitudeRecursiveGaussianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKGradientMagnitudeRecursiveGaussianImage::humanName() const
{
  return "ITK Gradient MagnitudeRecursiveGaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKGradientMagnitudeRecursiveGaussianImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKGradientMagnitudeRecursiveGaussianImage", "ITKImageGradient", "ImageGradient"};
}

//------------------------------------------------------------------------------
Parameters ITKGradientMagnitudeRecursiveGaussianImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{}));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));
  params.insert(std::make_unique<Float64Parameter>(k_Sigma_Key, "Sigma", "", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "NormalizeAcrossScale", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKGradientMagnitudeRecursiveGaussianImage::clone() const
{
  return std::make_unique<ITKGradientMagnitudeRecursiveGaussianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKGradientMagnitudeRecursiveGaussianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto pSigma = filterArgs.value<float64>(k_Sigma_Key);
  auto pNormalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  PreflightResult preflightResult;
  std::vector<PreflightValue> preflightUpdatedValues;

  complex::Result<OutputActions> resultOutputActions = ITK::DataCheck<FilterOutputType>(dataStructure, pSelectedInputArray, pImageGeomPath, pOutputArrayPath);

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
Result<> ITKGradientMagnitudeRecursiveGaussianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                                 const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto pSigma = filterArgs.value<float64>(k_Sigma_Key);
  auto pNormalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);

  /****************************************************************************
   * Create the functor object that will instantiate the correct itk filter
   ***************************************************************************/
  ::ITKGradientMagnitudeRecursiveGaussianImageCreationFunctor itkFunctor = {pSigma, pNormalizeAcrossScale};

  /****************************************************************************
   * Associate the output image with the Image Geometry for Visualization
   ***************************************************************************/
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(pOutputArrayPath);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  return ITK::Execute<ITKGradientMagnitudeRecursiveGaussianImageCreationFunctor, FilterOutputType>(dataStructure, pSelectedInputArray, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
