#include "ITKMedianProjectionImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkMedianProjectionImageFilter.h>

using namespace complex;

namespace cxITKMedianProjectionImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKMedianProjectionImageFunctor
{
  uint32 projectionDimension = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::MedianProjectionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetProjectionDimension(projectionDimension);
    return filter;
  }
};
} // namespace cxITKMedianProjectionImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMedianProjectionImage::name() const
{
  return FilterTraits<ITKMedianProjectionImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMedianProjectionImage::className() const
{
  return FilterTraits<ITKMedianProjectionImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMedianProjectionImage::uuid() const
{
  return FilterTraits<ITKMedianProjectionImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMedianProjectionImage::humanName() const
{
  return "ITK Median Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMedianProjectionImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKMedianProjectionImage", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKMedianProjectionImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "ProjectionDimension", "", 0u));

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
IFilter::UniquePointer ITKMedianProjectionImage::clone() const
{
  return std::make_unique<ITKMedianProjectionImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMedianProjectionImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMedianProjectionImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMedianProjectionImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  cxITKMedianProjectionImage::ITKMedianProjectionImageFunctor itkFunctor = {projectionDimension};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKMedianProjectionImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
