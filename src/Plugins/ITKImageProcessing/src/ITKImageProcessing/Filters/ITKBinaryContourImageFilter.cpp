#include "ITKBinaryContourImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkBinaryContourImageFilter.h>

using namespace nx::core;

namespace cxITKBinaryContourImageFilter
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;

struct ITKBinaryContourImageFunctor
{
  bool fullyConnected = false;
  float64 backgroundValue = 0.0;
  float64 foregroundValue = 1.0;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::BinaryContourImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetFullyConnected(fullyConnected);
    filter->SetBackgroundValue(backgroundValue);
    filter->SetForegroundValue(foregroundValue);
    return filter;
  }
};
} // namespace cxITKBinaryContourImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKBinaryContourImageFilter::name() const
{
  return FilterTraits<ITKBinaryContourImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryContourImageFilter::className() const
{
  return FilterTraits<ITKBinaryContourImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryContourImageFilter::uuid() const
{
  return FilterTraits<ITKBinaryContourImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryContourImageFilter::humanName() const
{
  return "ITK Binary Contour Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryContourImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKBinaryContourImage", "ITKImageLabel", "ImageLabel"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryContourImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "BackgroundValue", "Set/Get the background value used to mark the pixels not on the border of the objects.", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "ForegroundValue", "Set/Get the foreground value used to identify the objects in the input and output images.", 1.0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Array Name",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryContourImageFilter::clone() const
{
  return std::make_unique<ITKBinaryContourImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryContourImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBinaryContourImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryContourImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);

  const cxITKBinaryContourImageFilter::ITKBinaryContourImageFunctor itkFunctor = {fullyConnected, backgroundValue, foregroundValue};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKBinaryContourImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
