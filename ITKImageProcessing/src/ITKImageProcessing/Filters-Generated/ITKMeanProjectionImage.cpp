#include "ITKMeanProjectionImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkMeanProjectionImageFilter.h>

using namespace complex;

namespace cxITKMeanProjectionImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;
template <class PixelT>
using FilterOutputT = double;

struct ITKMeanProjectionImageFunctor
{
  uint32 projectionDimension = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::MeanProjectionImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetProjectionDimension(projectionDimension);
    return filter;
  }
};
} // namespace cxITKMeanProjectionImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMeanProjectionImage::name() const
{
  return FilterTraits<ITKMeanProjectionImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMeanProjectionImage::className() const
{
  return FilterTraits<ITKMeanProjectionImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMeanProjectionImage::uuid() const
{
  return FilterTraits<ITKMeanProjectionImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMeanProjectionImage::humanName() const
{
  return "ITK Mean Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMeanProjectionImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKMeanProjectionImage", "ITKImageStatistics", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKMeanProjectionImage::parameters() const
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
IFilter::UniquePointer ITKMeanProjectionImage::clone() const
{
  return std::make_unique<ITKMeanProjectionImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMeanProjectionImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKMeanProjectionImage::ArrayOptionsT, cxITKMeanProjectionImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMeanProjectionImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);

  cxITKMeanProjectionImage::ITKMeanProjectionImageFunctor itkFunctor = {projectionDimension};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKMeanProjectionImage::ArrayOptionsT, cxITKMeanProjectionImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
