#include "ITKBinaryContourImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkBinaryContourImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;

struct ITKBinaryContourImageFunctor
{
  bool fullyConnected = false;
  float64 backgroundValue = 0.0;
  float64 foregroundValue = 1.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::BinaryContourImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetFullyConnected(fullyConnected);
    filter->SetBackgroundValue(backgroundValue);
    filter->SetForegroundValue(foregroundValue);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBinaryContourImage::name() const
{
  return FilterTraits<ITKBinaryContourImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryContourImage::className() const
{
  return FilterTraits<ITKBinaryContourImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryContourImage::uuid() const
{
  return FilterTraits<ITKBinaryContourImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryContourImage::humanName() const
{
  return "ITK::BinaryContourImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryContourImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKBinaryContourImage", "ITKImageLabel", "ImageLabel"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryContourImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "ForegroundValue", "", 1.0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryContourImage::clone() const
{
  return std::make_unique<ITKBinaryContourImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryContourImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryContourImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);

  ITKBinaryContourImageFunctor itkFunctor = {fullyConnected, backgroundValue, foregroundValue};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKBinaryContourImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
