#include "ITKMorphologicalWatershedFromMarkersImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include <itkMorphologicalWatershedFromMarkersImageFilter.h>

using namespace nx::core;

namespace cxITKMorphologicalWatershedFromMarkersImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKMorphologicalWatershedFromMarkersImageFunctor
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
} // namespace cxITKMorphologicalWatershedFromMarkersImage

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedFromMarkersImage::name() const
{
  return FilterTraits<ITKMorphologicalWatershedFromMarkersImage>::name;
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
  return "ITK Morphological Watershed From Markers Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMorphologicalWatershedFromMarkersImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMorphologicalWatershedFromMarkersImage", "ITKWatersheds", "Watersheds"};
}

//------------------------------------------------------------------------------
Parameters ITKMorphologicalWatershedFromMarkersImage::parameters() const
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
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMorphologicalWatershedFromMarkersImage::clone() const
{
  return std::make_unique<ITKMorphologicalWatershedFromMarkersImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMorphologicalWatershedFromMarkersImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMorphologicalWatershedFromMarkersImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMorphologicalWatershedFromMarkersImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKMorphologicalWatershedFromMarkersImage::ITKMorphologicalWatershedFromMarkersImageFunctor itkFunctor = {markWatershedLine, fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKMorphologicalWatershedFromMarkersImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
