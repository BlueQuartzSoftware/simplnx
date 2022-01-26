#include "ITKValuedRegionalMinimaImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkValuedRegionalMinimaImageFilter.h>

using namespace complex;

namespace
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKValuedRegionalMinimaImageFunctor
{
  template <class InputImageT, class OutputImageT>
  using FilterType = itk::ValuedRegionalMinimaImageFilter<InputImageT, OutputImageT>;

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
std::string ITKValuedRegionalMinimaImage::name() const
{
  return FilterTraits<ITKValuedRegionalMinimaImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKValuedRegionalMinimaImage::className() const
{
  return FilterTraits<ITKValuedRegionalMinimaImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKValuedRegionalMinimaImage::uuid() const
{
  return FilterTraits<ITKValuedRegionalMinimaImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKValuedRegionalMinimaImage::humanName() const
{
  return "ITK::ValuedRegionalMinimaImageFilter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKValuedRegionalMinimaImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKValuedRegionalMinimaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKValuedRegionalMinimaImage::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKValuedRegionalMinimaImage::clone() const
{
  return std::make_unique<ITKValuedRegionalMinimaImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKValuedRegionalMinimaImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKValuedRegionalMinimaImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  ITKValuedRegionalMinimaImageFunctor itkFunctor = {fullyConnected};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  auto result = ITK::Execute<ITKValuedRegionalMinimaImageFunctor, ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor);

  if(result.valid())
  {
    ITKValuedRegionalMinimaImageFunctor::Measurements measurements = std::move(result.value());
    measurements.report(messageHandler);
  }

  return ConvertResult(std::move(result));
}
} // namespace complex
