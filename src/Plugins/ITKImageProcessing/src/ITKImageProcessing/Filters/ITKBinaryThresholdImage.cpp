#include "ITKBinaryThresholdImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkBinaryThresholdImageFilter.h>

using namespace complex;

namespace cxITKBinaryThresholdImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint8;

struct ITKBinaryThresholdImageFunctor
{
  float64 lowerThreshold = 0.0;
  float64 upperThreshold = 255.0;
  uint8 insideValue = 1u;
  uint8 outsideValue = 0u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::BinaryThresholdImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
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
  params.insert(std::make_unique<Float64Parameter>(k_LowerThreshold_Key, "LowerThreshold", "The lower threshold that a pixel value could be and still be considered 'Inside Value'", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_UpperThreshold_Key, "UpperThreshold", "The upper threshold that a pixel value could be and still be considered 'Inside Value'", 255.0));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "InsideValue", "Set the 'inside' pixel value. The default value NumericTraits<OutputPixelType>::max()", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "OutsideValue", "Set the 'outside' pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() .", 0u));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
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
  auto lowerThreshold = filterArgs.value<float64>(k_LowerThreshold_Key);
  auto upperThreshold = filterArgs.value<float64>(k_UpperThreshold_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKBinaryThresholdImage::ArrayOptionsType, cxITKBinaryThresholdImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

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

  const IDataArray* inputArray = dataStructure.getDataAs<IDataArray>(selectedInputArray);
  if(inputArray->getDataFormat() != "")
  {
    return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", selectedInputArray.toString()));
  }

  cxITKBinaryThresholdImage::ITKBinaryThresholdImageFunctor itkFunctor = {lowerThreshold, upperThreshold, insideValue, outsideValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKBinaryThresholdImage::ArrayOptionsType, cxITKBinaryThresholdImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                shouldCancel);
}
} // namespace complex
