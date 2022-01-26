#include "ITKSaltAndPepperNoiseImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkSaltAndPepperNoiseImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKSaltAndPepperNoiseImageFunctor
{
  float64 probability = 0.01;
  uint32 seed = (uint32_t)itk::simple::sitkWallClock;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::SaltAndPepperNoiseImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetProbability(probability);
    filter->SetSeed(seed);
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKSaltAndPepperNoiseImage::name() const
{
  return FilterTraits<ITKSaltAndPepperNoiseImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKSaltAndPepperNoiseImage::className() const
{
  return FilterTraits<ITKSaltAndPepperNoiseImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSaltAndPepperNoiseImage::uuid() const
{
  return FilterTraits<ITKSaltAndPepperNoiseImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSaltAndPepperNoiseImage::humanName() const
{
  return "ITK::SaltAndPepperNoiseImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSaltAndPepperNoiseImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKSaltAndPepperNoiseImage", "ITKImageNoise", "ImageNoise"};
}

//------------------------------------------------------------------------------
Parameters ITKSaltAndPepperNoiseImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_Probability_Key, "Probability", "", 0.01));
  params.insert(std::make_unique<UInt32Parameter>(k_Seed_Key, "Seed", "", (uint32_t)itk::simple::sitkWallClock));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKSaltAndPepperNoiseImage::clone() const
{
  return std::make_unique<ITKSaltAndPepperNoiseImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSaltAndPepperNoiseImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto probability = filterArgs.value<float64>(k_Probability_Key);
  auto seed = filterArgs.value<uint32>(k_Seed_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSaltAndPepperNoiseImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto probability = filterArgs.value<float64>(k_Probability_Key);
  auto seed = filterArgs.value<uint32>(k_Seed_Key);

  ITKSaltAndPepperNoiseImageFunctor itkFunctor = {probability, seed};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<ITKSaltAndPepperNoiseImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);
}
} // namespace complex
