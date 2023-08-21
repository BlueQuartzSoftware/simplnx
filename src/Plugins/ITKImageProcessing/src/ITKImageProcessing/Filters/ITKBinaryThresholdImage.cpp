#include "ITKBinaryThresholdImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkBinaryThresholdImageFilter.h>

using namespace complex;

namespace cxITKBinaryThresholdImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

template <class PixelT>
using FilterOutputT = uint8;

struct ITKBinaryThresholdImageFunctor
{
  float64 lowerThreshold = 0.0;
  float64 upperThreshold = 255.0;
  uint8 insideValue = 1u;
  uint8 outsideValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::BinaryThresholdImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetLowerThreshold(lowerThreshold);
    filter->SetUpperThreshold(upperThreshold);
    filter->SetInsideValue(insideValue);
    filter->SetOutsideValue(outsideValue);
    return filter;
  }
};
} // namespace cxITKBinaryThresholdImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBinaryThresholdImage::name() const
{
  return FilterTraits<ITKBinaryThresholdImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryThresholdImage::className() const
{
  return FilterTraits<ITKBinaryThresholdImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryThresholdImage::uuid() const
{
  return FilterTraits<ITKBinaryThresholdImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryThresholdImage::humanName() const
{
  return "ITK Binary Threshold Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryThresholdImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKBinaryThresholdImage", "ITKThresholding", "Thresholding"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryThresholdImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_LowerThreshold_Key, "LowerThreshold", "Set the lower threshold. Must be lower than the upper threshold.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_UpperThreshold_Key, "UpperThreshold", "Set the upper threshold.", 255.0));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "InsideValue", "Set the 'inside' pixel value", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "OutsideValue", "Set the 'outside' pixel value", 0u));

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
IFilter::UniquePointer ITKBinaryThresholdImage::clone() const
{
  return std::make_unique<ITKBinaryThresholdImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryThresholdImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto lowerThreshold = filterArgs.value<float64>(k_LowerThreshold_Key);
  auto upperThreshold = filterArgs.value<float64>(k_UpperThreshold_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKBinaryThresholdImage::ArrayOptionsT, cxITKBinaryThresholdImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryThresholdImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto lowerThreshold = filterArgs.value<float64>(k_LowerThreshold_Key);
  auto upperThreshold = filterArgs.value<float64>(k_UpperThreshold_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);

  cxITKBinaryThresholdImage::ITKBinaryThresholdImageFunctor itkFunctor = {lowerThreshold, upperThreshold, insideValue, outsideValue};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKBinaryThresholdImage::ArrayOptionsT, cxITKBinaryThresholdImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
