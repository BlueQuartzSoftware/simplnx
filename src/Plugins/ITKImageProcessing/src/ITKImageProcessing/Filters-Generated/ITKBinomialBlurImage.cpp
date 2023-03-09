#include "ITKBinomialBlurImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkBinomialBlurImageFilter.h>

using namespace complex;

namespace cxITKBinomialBlurImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

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
} // namespace cxITKBinomialBlurImage

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
  return "ITK Binomial Blur Image Filter";
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
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<UInt32Parameter>(k_Repetitions_Key, "Repetitions", "", 1u));

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
IFilter::UniquePointer ITKBinomialBlurImage::clone() const
{
  return std::make_unique<ITKBinomialBlurImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinomialBlurImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto repetitions = filterArgs.value<uint32>(k_Repetitions_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBinomialBlurImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinomialBlurImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto repetitions = filterArgs.value<uint32>(k_Repetitions_Key);

  cxITKBinomialBlurImage::ITKBinomialBlurImageFunctor itkFunctor = {repetitions};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKBinomialBlurImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
