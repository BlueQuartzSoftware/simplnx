#include "ITKSinImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkSinImageFilter.h>

using namespace complex;

namespace cxITKSinImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
  //VectorPixelIDTypeList;

struct ITKSinImageFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::SinImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSinImage::name() const
{
  return FilterTraits<ITKSinImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKSinImage::className() const
{
  return FilterTraits<ITKSinImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSinImage::uuid() const
{
  return FilterTraits<ITKSinImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSinImage::humanName() const
{
  return "ITK Sin Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSinImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKSinImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKSinImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}), GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSinImage::clone() const
{
  return std::make_unique<ITKSinImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSinImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSinImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSinImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  

  cxITKSinImage::ITKSinImageFunctor itkFunctor = {};

// LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath); 
// LINK GEOMETRY OUTPUT STOP
  
  return ITK::Execute<cxITKSinImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
