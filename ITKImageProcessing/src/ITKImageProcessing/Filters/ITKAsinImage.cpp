#include "ITKAsinImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkAsinImageFilter.h>

using namespace complex;

namespace cxITKAsinImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKAsinImageFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::AsinImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    return filter;
  }
};
} // namespace cxITKAsinImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKAsinImage::name() const
{
  return FilterTraits<ITKAsinImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKAsinImage::className() const
{
  return FilterTraits<ITKAsinImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKAsinImage::uuid() const
{
  return FilterTraits<ITKAsinImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKAsinImage::humanName() const
{
  return "ITK Asin Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKAsinImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKAsinImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKAsinImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKAsinImage::clone() const
{
  return std::make_unique<ITKAsinImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKAsinImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKAsinImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKAsinImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  cxITKAsinImage::ITKAsinImageFunctor itkFunctor = {};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKAsinImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
