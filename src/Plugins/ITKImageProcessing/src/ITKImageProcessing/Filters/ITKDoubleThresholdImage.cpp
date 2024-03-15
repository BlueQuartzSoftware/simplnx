#include "ITKDoubleThresholdImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include <itkDoubleThresholdImageFilter.h>

using namespace nx::core;

namespace cxITKDoubleThresholdImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = uint8;

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
    using FilterType = itk::DoubleThresholdImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
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

namespace nx::core
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
  return {className(), "ITKImageProcessing", "ITKDoubleThresholdImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKDoubleThresholdImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Threshold1_Key, "Threshold1",
                                                   "Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The "
                                                   "two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4.",
                                                   0.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold2_Key, "Threshold2",
                                                   "Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The "
                                                   "two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4.",
                                                   1.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold3_Key, "Threshold3",
                                                   "Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The "
                                                   "two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4.",
                                                   254.0));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold4_Key, "Threshold4",
                                                   "Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The "
                                                   "two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4.",
                                                   255.0));
  params.insert(std::make_unique<UInt8Parameter>(k_InsideValue_Key, "InsideValue", "Set the 'inside' pixel value. The default value NumericTraits<OutputPixelType>::max()", 1u));
  params.insert(std::make_unique<UInt8Parameter>(k_OutsideValue_Key, "OutsideValue", "Set the 'outside' pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() .", 0u));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

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
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto threshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto threshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto threshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto threshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKDoubleThresholdImage::ArrayOptionsType, cxITKDoubleThresholdImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKDoubleThresholdImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto threshold1 = filterArgs.value<float64>(k_Threshold1_Key);
  auto threshold2 = filterArgs.value<float64>(k_Threshold2_Key);
  auto threshold3 = filterArgs.value<float64>(k_Threshold3_Key);
  auto threshold4 = filterArgs.value<float64>(k_Threshold4_Key);
  auto insideValue = filterArgs.value<uint8>(k_InsideValue_Key);
  auto outsideValue = filterArgs.value<uint8>(k_OutsideValue_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKDoubleThresholdImage::ITKDoubleThresholdImageFunctor itkFunctor = {threshold1, threshold2, threshold3, threshold4, insideValue, outsideValue, fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKDoubleThresholdImage::ArrayOptionsType, cxITKDoubleThresholdImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor,
                                                                                                                shouldCancel);
}
} // namespace nx::core
