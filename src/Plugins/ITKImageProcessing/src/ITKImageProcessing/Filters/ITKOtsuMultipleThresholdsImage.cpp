#include "ITKOtsuMultipleThresholdsImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkOtsuMultipleThresholdsImageFilter.h>

using namespace complex;

namespace cxITKOtsuMultipleThresholdsImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint8;

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
    using FilterType = itk::OtsuMultipleThresholdsImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
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
  params.insert(std::make_unique<UInt8Parameter>(k_NumberOfThresholds_Key, "NumberOfThresholds", "Set/Get the number of thresholds. Default is 1.", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_LabelOffset_Key, "LabelOffset", "Set/Get the offset which labels have to start from. Default is 0.", 0u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfHistogramBins_Key, "NumberOfHistogramBins", "Set/Get the number of histogram bins. Default is 128.", 128u));
  params.insert(std::make_unique<BoolParameter>(k_ValleyEmphasis_Key, "ValleyEmphasis", "Set/Get the use of valley emphasis. Default is false.", false));
  params.insert(
      std::make_unique<BoolParameter>(k_ReturnBinMidpoint_Key, "ReturnBinMidpoint", "Should the threshold value be mid-point of the bin or the maximum? Default is to return bin maximum.", false));

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
  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  auto valleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto returnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKOtsuMultipleThresholdsImage::ArrayOptionsType, cxITKOtsuMultipleThresholdsImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKOtsuMultipleThresholdsImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
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

  const IDataArray* inputArray = dataStructure.getDataAs<IDataArray>(selectedInputArray);
  if(inputArray->getDataFormat() != "")
  {
    return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", selectedInputArray.toString()));
  }

  cxITKOtsuMultipleThresholdsImage::ITKOtsuMultipleThresholdsImageFunctor itkFunctor = {numberOfThresholds, labelOffset, numberOfHistogramBins, valleyEmphasis, returnBinMidpoint};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKOtsuMultipleThresholdsImage::ArrayOptionsType, cxITKOtsuMultipleThresholdsImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                              itkFunctor, shouldCancel);
}
} // namespace complex
