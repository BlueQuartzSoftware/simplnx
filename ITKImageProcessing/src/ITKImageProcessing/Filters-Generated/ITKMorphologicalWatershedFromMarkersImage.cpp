#include "ITKMorphologicalWatershedFromMarkersImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkMorphologicalWatershedFromMarkersImageFilter.h>

using namespace complex;

namespace cxITKMorphologicalWatershedFromMarkersImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKMorphologicalWatershedFromMarkersImageFunctor
{
  bool markWatershedLine = true;
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::MorphologicalWatershedFromMarkersImageFilter<InputImageT, InputImageT2>;
    auto filter = FilterT::New();
    filter->SetMarkWatershedLine(markWatershedLine);
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKMorphologicalWatershedFromMarkersImage

namespace complex
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
  return {"ITKImageProcessing", "ITKMorphologicalWatershedFromMarkersImage", "ITKWatersheds", "Watersheds"};
}

//------------------------------------------------------------------------------
Parameters ITKMorphologicalWatershedFromMarkersImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_MarkWatershedLine_Key, "MarkWatershedLine", "", true));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

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
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMorphologicalWatershedFromMarkersImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMorphologicalWatershedFromMarkersImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  cxITKMorphologicalWatershedFromMarkersImage::ITKMorphologicalWatershedFromMarkersImageFunctor itkFunctor = {markWatershedLine, fullyConnected};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKMorphologicalWatershedFromMarkersImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
