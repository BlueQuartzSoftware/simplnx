#include "ITKShotNoiseImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkShotNoiseImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKShotNoiseImageFunctor
{
  float64 scale = 1.0;
  uint32 seed = (uint32_t)itk::simple::sitkWallClock;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::ShotNoiseImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetScale(scale);
    filter->SetSeed(seed);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKShotNoiseImage::name() const
{
  return FilterTraits<ITKShotNoiseImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKShotNoiseImage::className() const
{
  return FilterTraits<ITKShotNoiseImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKShotNoiseImage::uuid() const
{
  return FilterTraits<ITKShotNoiseImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKShotNoiseImage::humanName() const
{
  return "ITK Shot Noise Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKShotNoiseImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKShotNoiseImage", "ITKImageNoise", "ImageNoise"};
}

//------------------------------------------------------------------------------
Parameters ITKShotNoiseImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "The image data that will be processed by this filter.", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "The result of the processing will be stored in this Data Array.", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_Scale_Key, "Scale", "", 1.0));
  params.insert(std::make_unique<UInt32Parameter>(k_Seed_Key, "Seed", "", (uint32_t)itk::simple::sitkWallClock));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKShotNoiseImage::clone() const
{
  return std::make_unique<ITKShotNoiseImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKShotNoiseImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto scale = filterArgs.value<float64>(k_Scale_Key);
  auto seed = filterArgs.value<uint32>(k_Seed_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKShotNoiseImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto scale = filterArgs.value<float64>(k_Scale_Key);
  auto seed = filterArgs.value<uint32>(k_Seed_Key);

  ITKShotNoiseImageFunctor itkFunctor = {scale, seed};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKShotNoiseImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
