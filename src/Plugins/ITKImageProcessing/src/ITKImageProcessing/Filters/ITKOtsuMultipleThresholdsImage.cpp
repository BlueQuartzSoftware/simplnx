#include "ITKOtsuMultipleThresholdsImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkOtsuMultipleThresholdsImageFilter.h>

using namespace complex;

namespace cxITKOtsuMultipleThresholdsImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

template <class PixelT>
using FilterOutputT = uint8;

struct ITKOtsuMultipleThresholdsImageFunctor
{
  uint8 numberOfThresholds = 1u;
  uint8 labelOffset = 0u;
  uint32 numberOfHistogramBins = 128u;
  bool valleyEmphasis = false;
  bool returnBinMidpoint = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::OtsuMultipleThresholdsImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetNumberOfThresholds(numberOfThresholds);
    filter->SetLabelOffset(labelOffset);
    filter->SetNumberOfHistogramBins(numberOfHistogramBins);
    filter->SetValleyEmphasis(valleyEmphasis);
    filter->SetReturnBinMidpoint(returnBinMidpoint);
    return filter;
  }
};
} // namespace cxITKOtsuMultipleThresholdsImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImage::name() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImage::className() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKOtsuMultipleThresholdsImage::uuid() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImage::humanName() const
{
  return "ITK Otsu Multiple Thresholds Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKOtsuMultipleThresholdsImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKOtsuMultipleThresholdsImage", "ITKThresholding", "Thresholding"};
}

//------------------------------------------------------------------------------
Parameters ITKOtsuMultipleThresholdsImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt8Parameter>(k_NumberOfThresholds_Key, "NumberOfThresholds", "The number of thresholds", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_LabelOffset_Key, "LabelOffset", "The offset which labels have to start from", 0u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfHistogramBins_Key, "NumberOfHistogramBins", "The number of histogram bins.", 128u));
  params.insert(std::make_unique<BoolParameter>(k_ValleyEmphasis_Key, "ValleyEmphasis", "Whether or not to use the valley emphasis algorithm", false));
  params.insert(std::make_unique<BoolParameter>(k_ReturnBinMidpoint_Key, "ReturnBinMidpoint", "", false));

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
IFilter::UniquePointer ITKOtsuMultipleThresholdsImage::clone() const
{
  return std::make_unique<ITKOtsuMultipleThresholdsImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKOtsuMultipleThresholdsImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  auto valleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto returnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKOtsuMultipleThresholdsImage::ArrayOptionsT, cxITKOtsuMultipleThresholdsImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKOtsuMultipleThresholdsImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  auto valleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto returnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  cxITKOtsuMultipleThresholdsImage::ITKOtsuMultipleThresholdsImageFunctor itkFunctor = {numberOfThresholds, labelOffset, numberOfHistogramBins, valleyEmphasis, returnBinMidpoint};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKOtsuMultipleThresholdsImage::ArrayOptionsT, cxITKOtsuMultipleThresholdsImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                        shouldCancel);
}
} // namespace complex
