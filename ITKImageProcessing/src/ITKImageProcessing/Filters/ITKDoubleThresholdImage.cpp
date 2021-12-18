#include "ITKDoubleThresholdImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkDoubleThresholdImageFilter.h>

namespace
{
struct ITKDoubleThresholdImageFilterCreationFunctor
{
  float64 m_Threshold1;
  float64 m_Threshold2;
  float64 m_Threshold3;
  float64 m_Threshold4;
  int32 m_InsideValue;
  int32 m_OutsideValue;
  bool m_FullyConnected;
  template <typename InputImageType, typename OutputImageType, unsigned int Dimension>
  auto operator()() const
  {
    typedef itk::DoubleThresholdImageFilter<InputImageType, OutputImageType> FilterType;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetThreshold1(static_cast<double>(m_Threshold1));
    filter->SetThreshold2(static_cast<double>(m_Threshold2));
    filter->SetThreshold3(static_cast<double>(m_Threshold3));
    filter->SetThreshold4(static_cast<double>(m_Threshold4));
    filter->SetInsideValue(static_cast<uint8_t>(m_InsideValue));
    filter->SetOutsideValue(static_cast<uint8_t>(m_OutsideValue));
    filter->SetFullyConnected(static_cast<bool>(m_FullyConnected));
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
  return "ITK::Double Threshold Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKDoubleThresholdImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK BiasCorrection"};
}

//------------------------------------------------------------------------------
Parameters ITKDoubleThresholdImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_Threshold1_Key, "Threshold1", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold2_Key, "Threshold2", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold3_Key, "Threshold3", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold4_Key, "Threshold4", "", 2.3456789));
  params.insert(std::make_unique<Int32Parameter>(k_InsideValue_Key, "InsideValue", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_OutsideValue_Key, "OutsideValue", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Filtered Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKDoubleThresholdImage::clone() const
{
  return std::make_unique<ITKDoubleThresholdImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKDoubleThresholdImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pThreshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto pThreshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto pThreshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto pThreshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto pInsideValue = filterArgs.value<int32>(k_InsideValue_Key);
  auto pOutsideValue = filterArgs.value<int32>(k_OutsideValue_Key);
  auto pFullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
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
Result<> ITKDoubleThresholdImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pThreshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto pThreshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto pThreshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto pThreshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto pInsideValue = filterArgs.value<int32>(k_InsideValue_Key);
  auto pOutsideValue = filterArgs.value<int32>(k_OutsideValue_Key);
  auto pFullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKDoubleThresholdImageFilterCreationFunctor itkFunctor;
  itkFunctor.m_Threshold1 = pThreshold1;
  itkFunctor.m_Threshold2 = pThreshold2;
  itkFunctor.m_Threshold3 = pThreshold3;
  itkFunctor.m_Threshold4 = pThreshold4;
  itkFunctor.m_InsideValue = pInsideValue;
  itkFunctor.m_OutsideValue = pOutsideValue;
  itkFunctor.m_FullyConnected = pFullyConnected;

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
