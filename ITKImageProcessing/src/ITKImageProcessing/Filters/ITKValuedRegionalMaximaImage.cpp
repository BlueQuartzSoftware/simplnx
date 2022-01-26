#include "ITKValuedRegionalMaximaImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkValuedRegionalMaximaImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKValuedRegionalMaximaImageFunctor
{
  template <class InputImageT, class OutputImageT>
  using FilterType = itk::ValuedRegionalMaximaImageFilter<InputImageT, OutputImageT>;

  struct Measurements
  {
    bool flat = {};

    void report(const IFilter::MessageHandler& messageHandler) const
    {
      messageHandler(IFilter::Message::Type::Info, fmt::format("Flat = {}", flat));
    }
  };

  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = FilterType<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  Measurements getMeasurements(const FilterType<InputImageT, OutputImageT>& filter) const
  {
    return {filter.GetFlat()};
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImage::name() const
{
  return FilterTraits<ITKValuedRegionalMaximaImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImage::className() const
{
  return FilterTraits<ITKValuedRegionalMaximaImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKValuedRegionalMaximaImage::uuid() const
{
  return FilterTraits<ITKValuedRegionalMaximaImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImage::humanName() const
{
  return "ITK::ValuedRegionalMaximaImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKValuedRegionalMaximaImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKValuedRegionalMaximaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKValuedRegionalMaximaImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKValuedRegionalMaximaImage::clone() const
{
  return std::make_unique<ITKValuedRegionalMaximaImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKValuedRegionalMaximaImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKValuedRegionalMaximaImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  ITKValuedRegionalMaximaImageFunctor itkFunctor = {fullyConnected};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  auto result = ITK::Execute<ITKValuedRegionalMaximaImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);

  if(result.valid())
  {
    ITKValuedRegionalMaximaImageFunctor::Measurements measurements = std::move(result.value());
    measurements.report(messageHandler);
  }

  return ConvertResult(std::move(result));
}
} // namespace complex
