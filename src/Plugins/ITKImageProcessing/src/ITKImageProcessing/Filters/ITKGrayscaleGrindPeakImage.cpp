#include "ITKGrayscaleGrindPeakImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"


#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkGrayscaleGrindPeakImageFilter.h>

using namespace complex;

namespace cxITKGrayscaleGrindPeakImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKGrayscaleGrindPeakImageFunctor
{
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::GrayscaleGrindPeakImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKGrayscaleGrindPeakImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKGrayscaleGrindPeakImage::name() const
{
  return FilterTraits<ITKGrayscaleGrindPeakImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKGrayscaleGrindPeakImage::className() const
{
  return FilterTraits<ITKGrayscaleGrindPeakImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKGrayscaleGrindPeakImage::uuid() const
{
  return FilterTraits<ITKGrayscaleGrindPeakImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKGrayscaleGrindPeakImage::humanName() const
{
  return "ITK Grayscale Grind Peak Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKGrayscaleGrindPeakImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKGrayscaleGrindPeakImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKGrayscaleGrindPeakImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKGrayscaleGrindPeakImage::clone() const
{
  return std::make_unique<ITKGrayscaleGrindPeakImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKGrayscaleGrindPeakImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKGrayscaleGrindPeakImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKGrayscaleGrindPeakImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKGrayscaleGrindPeakImage::ITKGrayscaleGrindPeakImageFunctor itkFunctor = {fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath); 
  
  return ITK::Execute<cxITKGrayscaleGrindPeakImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
