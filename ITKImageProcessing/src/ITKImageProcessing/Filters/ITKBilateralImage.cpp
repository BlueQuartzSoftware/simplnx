#include "ITKBilateralImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkBilateralImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKBilateralImageFunctor
{
  float64 domainSigma = 4.0;
  float64 rangeSigma = 50.0;
  uint32 numberOfRangeGaussianSamples = 100u;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::BilateralImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetDomainSigma(domainSigma);
    filter->SetRangeSigma(rangeSigma);
    filter->SetNumberOfRangeGaussianSamples(numberOfRangeGaussianSamples);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBilateralImage::name() const
{
  return FilterTraits<ITKBilateralImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBilateralImage::className() const
{
  return FilterTraits<ITKBilateralImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBilateralImage::uuid() const
{
  return FilterTraits<ITKBilateralImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBilateralImage::humanName() const
{
  return "ITK::BilateralImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBilateralImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKBilateralImage", "ITKImageFeature", "ImageFeature"};
}

//------------------------------------------------------------------------------
Parameters ITKBilateralImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_DomainSigma_Key, "DomainSigma", "", 4.0));
  params.insert(std::make_unique<Float64Parameter>(k_RangeSigma_Key, "RangeSigma", "", 50.0));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfRangeGaussianSamples_Key, "NumberOfRangeGaussianSamples", "", 100u));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBilateralImage::clone() const
{
  return std::make_unique<ITKBilateralImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBilateralImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto domainSigma = filterArgs.value<float64>(k_DomainSigma_Key);
  auto rangeSigma = filterArgs.value<float64>(k_RangeSigma_Key);
  auto numberOfRangeGaussianSamples = filterArgs.value<uint32>(k_NumberOfRangeGaussianSamples_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBilateralImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto domainSigma = filterArgs.value<float64>(k_DomainSigma_Key);
  auto rangeSigma = filterArgs.value<float64>(k_RangeSigma_Key);
  auto numberOfRangeGaussianSamples = filterArgs.value<uint32>(k_NumberOfRangeGaussianSamples_Key);

  ITKBilateralImageFunctor itkFunctor = {domainSigma, rangeSigma, numberOfRangeGaussianSamples};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKBilateralImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
