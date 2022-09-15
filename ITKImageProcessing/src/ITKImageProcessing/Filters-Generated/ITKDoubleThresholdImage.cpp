#include "ITKDoubleThresholdImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkDoubleThresholdImageFilter.h>

using namespace complex;

namespace cxITKDoubleThresholdImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputT = uint8;

struct ITKDoubleThresholdImageFunctor
{
  float64 threshold1 = 0.0;
  float64 threshold2 = 1.0;
  float64 threshold3 = 254.0;
  float64 threshold4 = 255.0;
  uint8 insideValue = 1u;
  uint8 outsideValue = 0u;
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::DoubleThresholdImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetThreshold1(threshold1);
    filter->SetThreshold2(threshold2);
    filter->SetThreshold3(threshold3);
    filter->SetThreshold4(threshold4);
    filter->SetInsideValue(insideValue);
    filter->SetOutsideValue(outsideValue);
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKDoubleThresholdImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKDoubleThresholdImage::name() const
{
  return FilterTraits<ITKDoubleThresholdImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKDoubleThresholdImage::className() const
{
  return FilterTraits<ITKDoubleThresholdImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKDoubleThresholdImage::uuid() const
{
  return FilterTraits<ITKDoubleThresholdImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKDoubleThresholdImage::humanName() const
{
  return "ITK Double Threshold Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKDoubleThresholdImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKDoubleThresholdImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKDoubleThresholdImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Threshold1_Key, "Threshold1", "", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold2_Key, "Threshold2", "", 1.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold3_Key, "Threshold3", "", 254.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold4_Key, "Threshold4", "", 255.0));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "InsideValue", "", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "OutsideValue", "", 0u));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKDoubleThresholdImage::clone() const
{
  return std::make_unique<ITKDoubleThresholdImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKDoubleThresholdImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto threshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto threshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto threshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto threshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKDoubleThresholdImage::ArrayOptionsT, cxITKDoubleThresholdImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKDoubleThresholdImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto threshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto threshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto threshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto threshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  cxITKDoubleThresholdImage::ITKDoubleThresholdImageFunctor itkFunctor = {threshold1, threshold2, threshold3, threshold4, insideValue, outsideValue, fullyConnected};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKDoubleThresholdImage::ArrayOptionsT, cxITKDoubleThresholdImage::FilterOutputT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
