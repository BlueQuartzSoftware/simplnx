#include "ITKRelabelComponentImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <fmt/ranges.h>

#include <itkRelabelComponentImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;

struct ITKRelabelComponentImageFunctor
{
  template <class InputImageT, class OutputImageT>
  using FilterType = itk::RelabelComponentImageFilter<InputImageT, OutputImageT>;

  struct Measurements
  {
    /**
     * @brief Get the number of objects in the image. This information is only valid after the filter has executed.
     */
    uint64 numberOfObjects = {};

    /**
     * @brief Get the original number of objects in the image before small objects were discarded. This information is only valid after the filter has executed. If the caller has not specified a
     * minimum object size, OriginalNumberOfObjects is the same as NumberOfObjects.
     */
    uint64 originalNumberOfObjects = {};

    /**
     * @brief Get the size of each object in physical space (in units of pixel size). This information is only valid after the filter has executed. Size of the background is not calculated. Size of
     * object #1 is GetSizeOfObjectsInPhysicalUnits() [0]. Size of object #2 is GetSizeOfObjectsInPhysicalUnits() [1]. Etc.
     */
    std::vector<float32> sizeOfObjectsInPhysicalUnits;

    /**
     * @brief Get the size of each object in pixels. This information is only valid after the filter has executed. Size of the background is not calculated. Size of object #1 is
     * GetSizeOfObjectsInPixels() [0]. Size of object #2 is GetSizeOfObjectsInPixels() [1]. Etc.
     */
    std::vector<uint64> sizeOfObjectsInPixels;

    void report(const IFilter::MessageHandler& messageHandler) const
    {
      messageHandler(IFilter::Message::Type::Info, fmt::format("NumberOfObjects = {}", numberOfObjects));
      messageHandler(IFilter::Message::Type::Info, fmt::format("OriginalNumberOfObjects = {}", originalNumberOfObjects));
      messageHandler(IFilter::Message::Type::Info, fmt::format("SizeOfObjectsInPhysicalUnits = {}", sizeOfObjectsInPhysicalUnits));
      messageHandler(IFilter::Message::Type::Info, fmt::format("SizeOfObjectsInPixels = {}", sizeOfObjectsInPixels));
    }
  };

  uint64 minimumObjectSize = 0u;
  bool sortByObjectSize = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = FilterType<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetMinimumObjectSize(minimumObjectSize);
    filter->SetSortByObjectSize(sortByObjectSize);
    return filter;
  }

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  Measurements getMeasurements(const FilterType<InputImageT, OutputImageT>& filter) const
  {
    return {filter.GetNumberOfObjects(), filter.GetOriginalNumberOfObjects(), filter.GetSizeOfObjectsInPhysicalUnits(), filter.GetSizeOfObjectsInPixels()};
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKRelabelComponentImage::name() const
{
  return FilterTraits<ITKRelabelComponentImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKRelabelComponentImage::className() const
{
  return FilterTraits<ITKRelabelComponentImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKRelabelComponentImage::uuid() const
{
  return FilterTraits<ITKRelabelComponentImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKRelabelComponentImage::humanName() const
{
  return "ITK::RelabelComponentImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKRelabelComponentImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKRelabelComponentImage", "ITKConnectedComponents", "ConnectedComponents"};
}

//------------------------------------------------------------------------------
Parameters ITKRelabelComponentImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<UInt64Parameter>(k_MinimumObjectSize_Key, "MinimumObjectSize", "", 0u));
  params.insert(std::make_unique<BoolParameter>(k_SortByObjectSize_Key, "SortByObjectSize", "", true));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKRelabelComponentImage::clone() const
{
  return std::make_unique<ITKRelabelComponentImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKRelabelComponentImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKRelabelComponentImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);

  ITKRelabelComponentImageFunctor itkFunctor = {minimumObjectSize, sortByObjectSize};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  auto result = ITK::Execute<ITKRelabelComponentImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);

  if(result.valid())
  {
    ITKRelabelComponentImageFunctor::Measurements measurements = std::move(result.value());
    measurements.report(messageHandler);
  }

  return ConvertResult(std::move(result));
}
} // namespace complex
