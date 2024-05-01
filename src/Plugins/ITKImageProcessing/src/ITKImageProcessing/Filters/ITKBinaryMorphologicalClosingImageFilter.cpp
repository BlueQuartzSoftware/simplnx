#include "ITKBinaryMorphologicalClosingImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkBinaryMorphologicalClosingImageFilter.h>

using namespace nx::core;

namespace cxITKBinaryMorphologicalClosingImageFilter
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;

struct ITKBinaryMorphologicalClosingImageFunctor
{
  std::vector<uint32> kernelRadius = {1, 1, 1};
  itk::simple::KernelEnum kernelType = itk::simple::sitkBall;
  float64 foregroundValue = 1.0;
  bool safeBorder = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::BinaryMorphologicalClosingImageFilter<InputImageT, OutputImageT, itk::FlatStructuringElement<InputImageT::ImageDimension>>;
    auto filter = FilterType::New();
    auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);
    filter->SetKernel(kernel);
    filter->SetForegroundValue(foregroundValue);
    filter->SetSafeBorder(safeBorder);
    return filter;
  }
};
} // namespace cxITKBinaryMorphologicalClosingImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKBinaryMorphologicalClosingImageFilter::name() const
{
  return FilterTraits<ITKBinaryMorphologicalClosingImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKBinaryMorphologicalClosingImageFilter::className() const
{
  return FilterTraits<ITKBinaryMorphologicalClosingImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBinaryMorphologicalClosingImageFilter::uuid() const
{
  return FilterTraits<ITKBinaryMorphologicalClosingImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBinaryMorphologicalClosingImageFilter::humanName() const
{
  return "ITK Binary Morphological Closing Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBinaryMorphologicalClosingImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKBinaryMorphologicalClosingImage", "ITKBinaryMathematicalMorphology", "BinaryMathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKBinaryMorphologicalClosingImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorParameter<uint32>>(k_KernelRadius_Key, "KernelRadius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                          std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "KernelType", "Set the kernel or structuring element used for the morphology.", static_cast<uint64>(itk::simple::sitkBall),
                                                   ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(
      std::make_unique<Float64Parameter>(k_ForegroundValue_Key, "ForegroundValue", "Set the value in the image to consider as 'foreground'. Defaults to maximum value of InputPixelType.", 1.0));
  params.insert(std::make_unique<BoolParameter>(k_SafeBorder_Key, "SafeBorder", "A safe border is added to input image to avoid borders effects and remove it once the closing is done", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBinaryMorphologicalClosingImageFilter::clone() const
{
  return std::make_unique<ITKBinaryMorphologicalClosingImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBinaryMorphologicalClosingImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto safeBorder = filterArgs.value<bool>(k_SafeBorder_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKBinaryMorphologicalClosingImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKBinaryMorphologicalClosingImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorParameter<uint32>::ValueType>(k_KernelRadius_Key);
  auto kernelType = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>(k_KernelType_Key));
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto safeBorder = filterArgs.value<bool>(k_SafeBorder_Key);

  const cxITKBinaryMorphologicalClosingImageFilter::ITKBinaryMorphologicalClosingImageFunctor itkFunctor = {kernelRadius, kernelType, foregroundValue, safeBorder};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKBinaryMorphologicalClosingImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
