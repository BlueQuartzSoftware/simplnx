#include "ITKBinomialBlurImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkBinomialBlurImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKBinomialBlurImageFunctor
{
  uint32 repetitions = 1u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::BinomialBlurImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetRepetitions(repetitions);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBinomialBlurImage::name() const
{
  return FilterTraits<ITKBinomialBlurImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinomialBlurImage::className() const
{
  return FilterTraits<ITKBinomialBlurImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinomialBlurImage::uuid() const
{
  return FilterTraits<ITKBinomialBlurImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinomialBlurImage::humanName() const
{
  return "ITK::BinomialBlurImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinomialBlurImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKBinomialBlurImage", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKBinomialBlurImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<UInt32Parameter>(k_Repetitions_Key, "Repetitions", "", 1u));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinomialBlurImage::clone() const
{
  return std::make_unique<ITKBinomialBlurImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinomialBlurImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto repetitions = filterArgs.value<uint32>(k_Repetitions_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinomialBlurImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto repetitions = filterArgs.value<uint32>(k_Repetitions_Key);

  ITKBinomialBlurImageFunctor itkFunctor = {repetitions};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKBinomialBlurImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
