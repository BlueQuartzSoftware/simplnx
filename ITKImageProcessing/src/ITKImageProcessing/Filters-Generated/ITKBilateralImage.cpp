#include "ITKBilateralImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkBilateralImageFilter.h>

using namespace complex;

namespace cxITKBilateralImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

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
} // namespace cxITKBilateralImage

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
  return "ITK Bilateral Image Filter";
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
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_DomainSigma_Key, "DomainSigma", "", 4.0));
  params.insert(std::make_unique<Float64Parameter>(k_RangeSigma_Key, "RangeSigma", "", 50.0));
  params.insert(std::make_unique<UInt32Parameter>(k_NumberOfRangeGaussianSamples_Key, "NumberOfRangeGaussianSamples", "", 100u));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBilateralImage::clone() const
{
  return std::make_unique<ITKBilateralImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBilateralImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto domainSigma = filterArgs.value<float64>(k_DomainSigma_Key);
  auto rangeSigma = filterArgs.value<float64>(k_RangeSigma_Key);
  auto numberOfRangeGaussianSamples = filterArgs.value<uint32>(k_NumberOfRangeGaussianSamples_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBilateralImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBilateralImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto domainSigma = filterArgs.value<float64>(k_DomainSigma_Key);
  auto rangeSigma = filterArgs.value<float64>(k_RangeSigma_Key);
  auto numberOfRangeGaussianSamples = filterArgs.value<uint32>(k_NumberOfRangeGaussianSamples_Key);

  cxITKBilateralImage::ITKBilateralImageFunctor itkFunctor = {domainSigma, rangeSigma, numberOfRangeGaussianSamples};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKBilateralImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
