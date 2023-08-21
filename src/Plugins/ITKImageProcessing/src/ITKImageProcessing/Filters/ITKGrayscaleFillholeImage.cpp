#include "ITKGrayscaleFillholeImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <itkGrayscaleFillholeImageFilter.h>

using namespace complex;

namespace cxITKGrayscaleFillholeImage
{
using ArrayOptionsT = ITK::ScalarPixelIdTypeList;

struct ITKGrayscaleFillholeImageFunctor
{
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::GrayscaleFillholeImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKGrayscaleFillholeImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKGrayscaleFillholeImage::name() const
{
  return FilterTraits<ITKGrayscaleFillholeImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKGrayscaleFillholeImage::className() const
{
  return FilterTraits<ITKGrayscaleFillholeImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKGrayscaleFillholeImage::uuid() const
{
  return FilterTraits<ITKGrayscaleFillholeImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKGrayscaleFillholeImage::humanName() const
{
  return "ITK Grayscale Fillhole Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKGrayscaleFillholeImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKGrayscaleFillholeImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKGrayscaleFillholeImage::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(
      k_FullyConnected_Key, "FullyConnected",
      "Whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. For objects that are 1 pixel wide, use FullyConnectedOn.", false));
  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::GetAllDataTypes()));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKGrayscaleFillholeImage::clone() const
{
  return std::make_unique<ITKGrayscaleFillholeImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKGrayscaleFillholeImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKGrayscaleFillholeImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKGrayscaleFillholeImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  cxITKGrayscaleFillholeImage::ITKGrayscaleFillholeImageFunctor itkFunctor = {fullyConnected};

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  itk::ProgressObserver::Pointer progressObs = itk::ProgressObserver::New(messageHandler);
  progressObs->setMessagePrefix("Processing Grayscale Fill Hole Image");

  return ITK::Execute<cxITKGrayscaleFillholeImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel, progressObs);
}
} // namespace complex
