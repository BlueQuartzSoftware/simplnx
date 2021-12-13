#include "ITKBinaryContourImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkBinaryContourImageFilter.h>
namespace
{
struct ITKBinaryContourImageFilterCreationFunctor
{
  bool m_FullyConnected;
  float64 m_BackgroundValue;
  float64 m_ForegroundValue;

  template <class InputImageType, class OutputImageType>
  auto operator()() const
  {
    using FilterType = itk::BinaryContourImageFilter<InputImageType, OutputImageType>;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetFullyConnected(m_FullyConnected);
    filter->SetBackgroundValue(m_BackgroundValue);
    filter->SetForegroundValue(m_ForegroundValue);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBinaryContourImage::name() const
{
  return FilterTraits<ITKBinaryContourImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKBinaryContourImage::className() const
{
  return FilterTraits<ITKBinaryContourImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryContourImage::uuid() const
{
  return FilterTraits<ITKBinaryContourImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryContourImage::humanName() const
{
  return "ITK::Binary Contour Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryContourImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK ImageLabel"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryContourImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "ForegroundValue", "", 2.3456789));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Filtered Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryContourImage::clone() const
{
  return std::make_unique<ITKBinaryContourImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryContourImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pBackgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto pForegroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
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
Result<> ITKBinaryContourImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pBackgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto pForegroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKBinaryContourImageFilterCreationFunctor itkFunctor;
  itkFunctor.m_FullyConnected = pFullyConnected;
  itkFunctor.m_BackgroundValue = pBackgroundValue;
  itkFunctor.m_ForegroundValue = pForegroundValue;

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
