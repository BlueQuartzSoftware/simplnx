#include "ITKDoubleThresholdImage.hpp"

/**
 * This filter only works with certain kinds of data. We
 * enable the types that the filter will compile against. The
 * Allowed PixelTypes as defined in SimpleITK are:
 *   BasicPixelIDTypeList
 * The filter defines the following output pixel types:
 *   uint8_t
 */
#define ITK_BASIC_PIXEL_ID_TYPE_LIST 1
#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1
#define ITK_ARRAY_HELPER_NAMESPACE DoubleThresholdImage

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkDoubleThresholdImageFilter.h>

using namespace complex;

namespace
{
/**
 * This filter uses a fixed output type.
 */
using FilterOutputType = uint8_t;

struct ITKDoubleThresholdImageCreationFunctor
{
  float64 pThreshold1 = 0.0;
  float64 pThreshold2 = 1.0;
  float64 pThreshold3 = 254.0;
  float64 pThreshold4 = 255.0;
  uint8_t pInsideValue = 1u;
  uint8_t pOutsideValue = 0u;
  bool pFullyConnected = false;

  template <class InputImageType, class OutputImageType, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::DoubleThresholdImageFilter<InputImageType, OutputImageType>;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetThreshold1(pThreshold1);
    filter->SetThreshold2(pThreshold2);
    filter->SetThreshold3(pThreshold3);
    filter->SetThreshold4(pThreshold4);
    filter->SetInsideValue(pInsideValue);
    filter->SetOutsideValue(pOutsideValue);
    filter->SetFullyConnected(pFullyConnected);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKDoubleThresholdImage::name() const
{
  return FilterTraits<ITKDoubleThresholdImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKDoubleThresholdImage::className() const
{
  return FilterTraits<ITKDoubleThresholdImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKDoubleThresholdImage::uuid() const
{
  return FilterTraits<ITKDoubleThresholdImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKDoubleThresholdImage::humanName() const
{
  return "ITK Double Threshold Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKDoubleThresholdImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKDoubleThresholdImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKDoubleThresholdImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "The image data that will be processed by this filter.", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "The result of the processing will be stored in this Data Array.", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold1_Key, "Threshold1", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold2_Key, "Threshold2", "", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold3_Key, "Threshold3", "", 254.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold4_Key, "Threshold4", "", 255.0));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "InsideValue", "", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "OutsideValue", "", 0u));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKDoubleThresholdImage::clone() const
{
  return std::make_unique<ITKDoubleThresholdImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKDoubleThresholdImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
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
  auto pThreshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto pThreshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto pThreshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto pThreshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto pInsideValue = filterArgs.value<uint8_t>(k_InsideValue_Key);
  auto pOutsideValue = filterArgs.value<uint8_t>(k_OutsideValue_Key);
  auto pFullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

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
Result<> ITKDoubleThresholdImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto pThreshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto pThreshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto pThreshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto pThreshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto pInsideValue = filterArgs.value<uint8_t>(k_InsideValue_Key);
  auto pOutsideValue = filterArgs.value<uint8_t>(k_OutsideValue_Key);
  auto pFullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  /****************************************************************************
   * Create the functor object that will instantiate the correct itk filter
   ***************************************************************************/
  ::ITKDoubleThresholdImageCreationFunctor itkFunctor = {pThreshold1, pThreshold2, pThreshold3, pThreshold4, pInsideValue, pOutsideValue, pFullyConnected};

  /****************************************************************************
   * Associate the output image with the Image Geometry for Visualization
   ***************************************************************************/
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(pOutputArrayPath);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  return ITK::Execute<ITKDoubleThresholdImageCreationFunctor, FilterOutputType>(dataStructure, pSelectedInputArray, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
