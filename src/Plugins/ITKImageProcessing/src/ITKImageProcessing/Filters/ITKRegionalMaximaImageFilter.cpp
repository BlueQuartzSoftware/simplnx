#include "ITKRegionalMaximaImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <ITKRegionalMaximaImageFilter.h>

using namespace nx::core;

namespace cxITKRegionalMaximaImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint32;

struct ITKRegionalMaximaImageFilterFunctor
{
  float64 backgroundValue = 0.0;
  float64 foregroundValue = 1.0;
  bool fullyConnected = false;
  bool flatIsMaxima = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::RegionalMaximaImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetBackgroundValue(backgroundValue);
    filter->SetForegroundValue(foregroundValue);
    filter->SetFullyConnected(fullyConnected);
    filter->SetFlatIsMaxima(flatIsMaxima);
    return filter;
  }
};
} // namespace cxITKRegionalMaximaImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKRegionalMaximaImageFilter::name() const
{
  return FilterTraits<ITKRegionalMaximaImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKRegionalMaximaImageFilter::className() const
{
  return FilterTraits<ITKRegionalMaximaImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKRegionalMaximaImageFilter::uuid() const
{
  return FilterTraits<ITKRegionalMaximaImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKRegionalMaximaImageFilter::humanName() const
{
  return "ITK Regional Maxima Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKRegionalMaximaImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKRegionalMaximaImageFilter", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKRegionalMaximaImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "Set/Get the value used as 'background' in the output image. Defaults to NumericTraits<PixelType>::NonpositiveMin() .", 0.0));
  params.insert(
      std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "Foreground Value", "Set/Get the value in the output image to consider as 'foreground'. Defaults to maximum value of PixelType.", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));
  params.insert(std::make_unique<BoolParameter>(k_FlatIsMaxima_Key, "Flat Is Maxima", "Set/Get whether a flat image must be considered as a maxima or not. Defaults to true.", true));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKRegionalMaximaImageFilter::clone() const
{
  return std::make_unique<ITKRegionalMaximaImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKRegionalMaximaImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto flatIsMaxima = filterArgs.value<bool>(k_FlatIsMaxima_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKRegionalMaximaImageFilter::ArrayOptionsType, cxITKRegionalMaximaImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKRegionalMaximaImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto flatIsMaxima = filterArgs.value<bool>(k_FlatIsMaxima_Key);

  const cxITKRegionalMaximaImageFilter::ITKRegionalMaximaImageFilterFunctor itkFunctor = {backgroundValue, foregroundValue, fullyConnected, flatIsMaxima};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKRegionalMaximaImageFilter::ArrayOptionsType, cxITKRegionalMaximaImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                          shouldCancel);
}
} // namespace nx::core
