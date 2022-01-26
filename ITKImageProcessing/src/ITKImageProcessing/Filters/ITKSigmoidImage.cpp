#include "ITKSigmoidImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkSigmoidImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKSigmoidImageFunctor
{
  float64 alpha = 1;
  float64 beta = 0;
  float64 outputMaximum = 255;
  float64 outputMinimum = 0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::SigmoidImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetAlpha(alpha);
    filter->SetBeta(beta);
    filter->SetOutputMaximum(outputMaximum);
    filter->SetOutputMinimum(outputMinimum);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSigmoidImage::name() const
{
  return FilterTraits<ITKSigmoidImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKSigmoidImage::className() const
{
  return FilterTraits<ITKSigmoidImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSigmoidImage::uuid() const
{
  return FilterTraits<ITKSigmoidImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSigmoidImage::humanName() const
{
  return "ITK::SigmoidImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSigmoidImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKSigmoidImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKSigmoidImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_Alpha_Key, "Alpha", "", 1));
  params.insert(std::make_unique<Float64Parameter>(k_Beta_Key, "Beta", "", 0));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximum_Key, "OutputMaximum", "", 255));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMinimum_Key, "OutputMinimum", "", 0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSigmoidImage::clone() const
{
  return std::make_unique<ITKSigmoidImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSigmoidImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto alpha = filterArgs.value<float64>(k_Alpha_Key);
  auto beta = filterArgs.value<float64>(k_Beta_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSigmoidImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto alpha = filterArgs.value<float64>(k_Alpha_Key);
  auto beta = filterArgs.value<float64>(k_Beta_Key);
  auto outputMaximum = filterArgs.value<float64>(k_OutputMaximum_Key);
  auto outputMinimum = filterArgs.value<float64>(k_OutputMinimum_Key);

  ITKSigmoidImageFunctor itkFunctor = {alpha, beta, outputMaximum, outputMinimum};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKSigmoidImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
