#include "ITKOtsuMultipleThresholdsImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkOtsuMultipleThresholdsImageFilter.h>

using namespace complex;

namespace
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
} // namespace

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
  return "ITK::OtsuMultipleThresholdsImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKOtsuMultipleThresholdsImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKOtsuMultipleThresholdsImage", "ITKThresholding", "Thresholding"};
}

//------------------------------------------------------------------------------
Parameters ITKOtsuMultipleThresholdsImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<UInt8Parameter>(k_NumberOfThresholds_Key, "NumberOfThresholds", "", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_LabelOffset_Key, "LabelOffset", "", 0u));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfHistogramBins_Key, "NumberOfHistogramBins", "", 128u));
  params.insert(std::make_unique<BoolParameter>(k_ValleyEmphasis_Key, "ValleyEmphasis", "", false));
  params.insert(std::make_unique<BoolParameter>(k_ReturnBinMidpoint_Key, "ReturnBinMidpoint", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKOtsuMultipleThresholdsImage::clone() const
{
  return std::make_unique<ITKOtsuMultipleThresholdsImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKOtsuMultipleThresholdsImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  auto valleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto returnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT, FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKOtsuMultipleThresholdsImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto numberOfThresholds = filterArgs.value<uint8>(k_NumberOfThresholds_Key);
  auto labelOffset = filterArgs.value<uint8>(k_LabelOffset_Key);
  auto numberOfHistogramBins = filterArgs.value<uint32>(k_NumberOfHistogramBins_Key);
  auto valleyEmphasis = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto returnBinMidpoint = filterArgs.value<bool>(k_ReturnBinMidpoint_Key);

  ITKOtsuMultipleThresholdsImageFunctor itkFunctor = {numberOfThresholds, labelOffset, numberOfHistogramBins, valleyEmphasis, returnBinMidpoint};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ArrayOptionsT, FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
