#include "ITKNormalizeImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkNormalizeImageFilter.h>

using namespace complex;

namespace cxITKNormalizeImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

template <class PixelT>
using FilterOutputT = typename itk::NumericTraits<PixelT>::RealType;

struct ITKNormalizeImageFunctor
{
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::NormalizeImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
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

  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

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

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKNormalizeImage::ArrayOptionsT, cxITKNormalizeImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

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

  cxITKNormalizeImage::ITKNormalizeImageFunctor itkFunctor = {};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKNormalizeImage::ArrayOptionsT, cxITKNormalizeImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
