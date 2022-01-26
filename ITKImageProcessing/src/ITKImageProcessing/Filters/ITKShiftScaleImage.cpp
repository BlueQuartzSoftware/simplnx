#include "ITKShiftScaleImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkShiftScaleImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKShiftScaleImageFunctor
{
  template <class InputImageT, class OutputImageT>
  using FilterType = itk::ShiftScaleImageFilter<InputImageT, OutputImageT>;

  struct Measurements
  {
    /**
     * @brief Get the number of pixels that underflowed and overflowed.
     */
    uint64 underflowCount = {};

    /**
     * @brief Get the number of pixels that underflowed and overflowed.
     */
    uint64 overflowCount = {};

    void report(const IFilter::MessageHandler& messageHandler) const
    {
      messageHandler(IFilter::Message::Type::Info, fmt::format("UnderflowCount = {}", underflowCount));
      messageHandler(IFilter::Message::Type::Info, fmt::format("OverflowCount = {}", overflowCount));
    }
  };

  float64 shift = 0;
  float64 scale = 1.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = FilterType<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetShift(shift);
    filter->SetScale(scale);
    return filter;
  }

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  Measurements getMeasurements(const FilterType<InputImageT, OutputImageT>& filter) const
  {
    return {filter.GetUnderflowCount(), filter.GetOverflowCount()};
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKShiftScaleImage::name() const
{
  return FilterTraits<ITKShiftScaleImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKShiftScaleImage::className() const
{
  return FilterTraits<ITKShiftScaleImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKShiftScaleImage::uuid() const
{
  return FilterTraits<ITKShiftScaleImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKShiftScaleImage::humanName() const
{
  return "ITK::ShiftScaleImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKShiftScaleImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKShiftScaleImage", "ITKImageIntensity", "ImageIntensity"};
}

//------------------------------------------------------------------------------
Parameters ITKShiftScaleImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<Float64Parameter>(k_Shift_Key, "Shift", "", 0));
  params.insert(std::make_unique<Float64Parameter>(k_Scale_Key, "Scale", "", 1.0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKShiftScaleImage::clone() const
{
  return std::make_unique<ITKShiftScaleImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKShiftScaleImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto shift = filterArgs.value<float64>(k_Shift_Key);
  auto scale = filterArgs.value<float64>(k_Scale_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKShiftScaleImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto shift = filterArgs.value<float64>(k_Shift_Key);
  auto scale = filterArgs.value<float64>(k_Scale_Key);

  ITKShiftScaleImageFunctor itkFunctor = {shift, scale};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  auto result = ITK::Execute<ITKShiftScaleImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);

  if(result.valid())
  {
    ITKShiftScaleImageFunctor::Measurements measurements = std::move(result.value());
    measurements.report(messageHandler);
  }

  return ConvertResult(std::move(result));
}
} // namespace complex
