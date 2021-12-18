#include "ITKMorphologicalWatershedFromMarkersImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkMorphologicalWatershedFromMarkersImageFilter.h>

namespace
{
struct ITKMorphologicalWatershedFromMarkersImageFilterCreationFunctor
{
  bool m_MarkWatershedLine;
  bool m_FullyConnected;
  DataPath m_MarkerCellArrayPath;
  template <typename InputImageType, typename OutputImageType, unsigned int Dimension>
  auto operator()() const
  {
    typedef itk::MorphologicalWatershedFromMarkersImageFilter<InputImageType, OutputImageType> FilterType;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetMarkWatershedLine(static_cast<bool>(m_MarkWatershedLine));
    filter->SetFullyConnected(static_cast<bool>(m_FullyConnected));
    try
    {
      DataArrayPath dap = getMarkerCellArrayPath();
      DataContainer::Pointer dcMarker = getMarkerContainerArray()->getDataContainer(dap.getDataContainerName());
      typedef itk::InPlaceDream3DDataToImageFilter<uint16_t, Dimension> toITKType;
      typename toITKType::Pointer toITK = toITKType::New();
      toITK->SetInput(dcMarker);
      toITK->SetInPlace(true);
      toITK->SetAttributeMatrixArrayName(dap.getAttributeMatrixName().toStdString());
      toITK->SetDataArrayName(dap.getDataArrayName().toStdString());
      toITK->Update();
      filter->SetMarkerImage(toITK->GetOutput());
    } catch(itk::ExceptionObject& err)
    {
      QString errorMessage = "ITK exception was thrown while converting marker image: %1";
      break;
    }
    return filter;
    m_MarkerContainerArray = nullptr; // Free the memory used by the casted marker image
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedFromMarkersImage::name() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedFromMarkersImage::className() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMorphologicalWatershedFromMarkersImage::uuid() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedFromMarkersImage::humanName() const
{
  return "ITK::Morphological Watershed From Markers Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMorphologicalWatershedFromMarkersImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Segmentation"};
}

//------------------------------------------------------------------------------
Parameters ITKMorphologicalWatershedFromMarkersImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_MarkWatershedLine_Key, "MarkWatershedLine", "", false));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MarkerCellArrayPath_Key, "Marker Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Filtered Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMorphologicalWatershedFromMarkersImage::clone() const
{
  return std::make_unique<ITKMorphologicalWatershedFromMarkersImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMorphologicalWatershedFromMarkersImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMarkWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto pFullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMarkerCellArrayPath = filterArgs.value<DataPath>(k_MarkerCellArrayPath_Key);
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
Result<> ITKMorphologicalWatershedFromMarkersImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                                const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMarkWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto pFullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMarkerCellArrayPath = filterArgs.value<DataPath>(k_MarkerCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKMorphologicalWatershedFromMarkersImageFilterCreationFunctor itkFunctor;
  itkFunctor.m_MarkWatershedLine = pMarkWatershedLine;
  itkFunctor.m_FullyConnected = pFullyConnected;
  itkFunctor.m_MarkerCellArrayPath = pMarkerCellArrayPath;

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
