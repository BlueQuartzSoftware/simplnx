#include "ITKNormalizeImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"


#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkNormalizeImageFilter.h>

using namespace complex;

namespace cxITKNormalizeImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
  //VectorPixelIDTypeList;
template<class PixelT>
using FilterOutputType = double;

struct ITKNormalizeImageFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::NormalizeImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    return filter;
  }
};
} // namespace cxITKNormalizeImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKNormalizeImage::name() const
{
  return FilterTraits<ITKNormalizeImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKNormalizeImage::className() const
{
  return FilterTraits<ITKNormalizeImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKNormalizeImage::uuid() const
{
  return FilterTraits<ITKNormalizeImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKNormalizeImage::humanName() const
{
  return "ITK Normalize Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKNormalizeImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKNormalizeImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKNormalizeImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKNormalizeImage::clone() const
{
  return std::make_unique<ITKNormalizeImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKNormalizeImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKNormalizeImage::ArrayOptionsType, cxITKNormalizeImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKNormalizeImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  

  const cxITKNormalizeImage::ITKNormalizeImageFunctor itkFunctor = {};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath); 
  
  return ITK::Execute<cxITKNormalizeImage::ArrayOptionsType, cxITKNormalizeImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
