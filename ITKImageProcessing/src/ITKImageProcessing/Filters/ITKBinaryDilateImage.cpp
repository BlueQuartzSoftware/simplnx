#include "ITKBinaryDilateImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkBinaryDilateImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;

struct ITKBinaryDilateImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;
  float64 backgroundValue = 0.0;
  float64 foregroundValue = 1.0;
  bool boundaryToForeground = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::BinaryDilateImageFilter<InputImageT, OutputImageT, itk::FlatStructuringElement<InputImageT::ImageDimension>>;
    auto filter = FilterT::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    filter->SetBackgroundValue(backgroundValue);
    filter->SetForegroundValue(foregroundValue);
    filter->SetBoundaryToForeground(boundaryToForeground);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBinaryDilateImage::name() const
{
  return FilterTraits<ITKBinaryDilateImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryDilateImage::className() const
{
  return FilterTraits<ITKBinaryDilateImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryDilateImage::uuid() const
{
  return FilterTraits<ITKBinaryDilateImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryDilateImage::humanName() const
{
  return "ITK::BinaryDilateImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryDilateImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKBinaryDilateImage", "ITKBinaryMathematicalMorphology", "BinaryMathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryDilateImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "KernelRadius", "", std::vector<uint32>(3, 1), std::vector<std::string>(3)));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "KernelType", "", static_cast<uint64>(itk::simple::sitkBall), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "ForegroundValue", "", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_BoundaryToForeground_Key, "BoundaryToForeground", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryDilateImage::clone() const
{
  return std::make_unique<ITKBinaryDilateImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryDilateImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto boundaryToForeground = filterArgs.value<bool>(k_BoundaryToForeground_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryDilateImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto boundaryToForeground = filterArgs.value<bool>(k_BoundaryToForeground_Key);

  ITKBinaryDilateImageFunctor itkFunctor = {kernelRadius, kernelType, backgroundValue, foregroundValue, boundaryToForeground};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKBinaryDilateImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
