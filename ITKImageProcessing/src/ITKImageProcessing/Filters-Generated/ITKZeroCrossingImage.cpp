#include "ITKZeroCrossingImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkZeroCrossingImageFilter.h>

using namespace complex;

namespace cxITKZeroCrossingImage
{
using ArrayOptionsT = ITK::SignedIntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = uint8;

struct ITKZeroCrossingImageFunctor
{
  uint8 foregroundValue = 1u;
  uint8 backgroundValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::ZeroCrossingImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetForegroundValue(foregroundValue);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace cxITKZeroCrossingImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKZeroCrossingImage::name() const
{
  return FilterTraits<ITKZeroCrossingImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKZeroCrossingImage::className() const
{
  return FilterTraits<ITKZeroCrossingImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKZeroCrossingImage::uuid() const
{
  return FilterTraits<ITKZeroCrossingImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKZeroCrossingImage::humanName() const
{
  return "ITK Zero Crossing Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKZeroCrossingImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKZeroCrossingImage", "ITKImageFeature", "ImageFeature"};
}

//------------------------------------------------------------------------------
Parameters ITKZeroCrossingImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<UInt8Parameter>(k_ForegroundValue_Key, "ForegroundValue", "", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 0u));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetSignedIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKZeroCrossingImage::clone() const
{
  return std::make_unique<ITKZeroCrossingImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKZeroCrossingImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto foregroundValue = filterArgs.value<uint8>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<uint8>(k_BackgroundValue_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKZeroCrossingImage::ArrayOptionsT, cxITKZeroCrossingImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKZeroCrossingImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto foregroundValue = filterArgs.value<uint8>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<uint8>(k_BackgroundValue_Key);

  cxITKZeroCrossingImage::ITKZeroCrossingImageFunctor itkFunctor = {foregroundValue, backgroundValue};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKZeroCrossingImage::ArrayOptionsT, cxITKZeroCrossingImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
