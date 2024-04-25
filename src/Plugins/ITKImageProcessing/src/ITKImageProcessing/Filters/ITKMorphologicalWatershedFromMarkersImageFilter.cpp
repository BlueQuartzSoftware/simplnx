#include "ITKMorphologicalWatershedFromMarkersImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include <ITKMorphologicalWatershedFromMarkersImageFilter.h>

using namespace nx::core;

namespace cxITKMorphologicalWatershedFromMarkersImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKMorphologicalWatershedFromMarkersImageFilterFunctor
{
  bool markWatershedLine = true;
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MorphologicalWatershedFromMarkersImageFilter<InputImageT, InputImageT2>;
    auto filter = FilterType::New();
    filter->SetMarkWatershedLine(markWatershedLine);
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKMorphologicalWatershedFromMarkersImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedFromMarkersImageFilter::name() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedFromMarkersImageFilter::className() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMorphologicalWatershedFromMarkersImageFilter::uuid() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedFromMarkersImageFilter::humanName() const
{
  return "ITK Morphological Watershed From Markers Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMorphologicalWatershedFromMarkersImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMorphologicalWatershedFromMarkersImageFilter", "ITKWatersheds", "Watersheds"};
}

//------------------------------------------------------------------------------
Parameters ITKMorphologicalWatershedFromMarkersImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(
      k_MarkWatershedLine_Key, "Mark Watershed Line",
      "Set/Get whether the watershed pixel must be marked or not. Default is true. Set it to false do not only avoid writing watershed pixels, it also decrease algorithm complexity.", true));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMorphologicalWatershedFromMarkersImageFilter::clone() const
{
  return std::make_unique<ITKMorphologicalWatershedFromMarkersImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMorphologicalWatershedFromMarkersImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMorphologicalWatershedFromMarkersImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMorphologicalWatershedFromMarkersImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKMorphologicalWatershedFromMarkersImageFilter::ITKMorphologicalWatershedFromMarkersImageFilterFunctor itkFunctor = {markWatershedLine, fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKMorphologicalWatershedFromMarkersImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
