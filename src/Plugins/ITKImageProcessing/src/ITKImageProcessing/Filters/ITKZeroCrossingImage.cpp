#include "ITKZeroCrossingImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkZeroCrossingImageFilter.h>

using namespace nx::core;

namespace cxITKZeroCrossingImage
{
using ArrayOptionsType = ITK::SignedScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint8;

struct ITKZeroCrossingImageFunctor
{
  uint8 foregroundValue = 1u;
  uint8 backgroundValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ZeroCrossingImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetForegroundValue(foregroundValue);
    filter->SetBackgroundValue(backgroundValue);
    return filter;
  }
};
} // namespace cxITKZeroCrossingImage

namespace nx::core
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
  return {className(), "ITKImageProcessing", "ITKZeroCrossingImage", "ITKImageFeature", "ImageFeature"};
}

//------------------------------------------------------------------------------
Parameters ITKZeroCrossingImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt8Parameter>(k_ForegroundValue_Key, "ForegroundValue", "Set/Get the label value for zero-crossing pixels.", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_BackgroundValue_Key, "BackgroundValue", "Set/Get the label value for non-zero-crossing pixels.", 0u));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetSignedScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

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
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto foregroundValue = filterArgs.value<uint8>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<uint8>(k_BackgroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKZeroCrossingImage::ArrayOptionsType, cxITKZeroCrossingImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKZeroCrossingImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto foregroundValue = filterArgs.value<uint8>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<uint8>(k_BackgroundValue_Key);

  const cxITKZeroCrossingImage::ITKZeroCrossingImageFunctor itkFunctor = {foregroundValue, backgroundValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKZeroCrossingImage::ArrayOptionsType, cxITKZeroCrossingImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
