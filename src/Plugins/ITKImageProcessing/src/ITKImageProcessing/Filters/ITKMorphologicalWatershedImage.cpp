#include "ITKMorphologicalWatershedImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"


#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkMorphologicalWatershedImageFilter.h>

using namespace complex;

namespace cxITKMorphologicalWatershedImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template<class PixelT>
using FilterOutputType = uint32;

struct ITKMorphologicalWatershedImageFunctor
{
  float64 level = 0.0;
  bool markWatershedLine = true;
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MorphologicalWatershedImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetLevel(level);
    filter->SetMarkWatershedLine(markWatershedLine);
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKMorphologicalWatershedImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedImage::name() const
{
  return FilterTraits<ITKMorphologicalWatershedImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedImage::className() const
{
  return FilterTraits<ITKMorphologicalWatershedImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMorphologicalWatershedImage::uuid() const
{
  return FilterTraits<ITKMorphologicalWatershedImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedImage::humanName() const
{
  return "ITK Morphological Watershed Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMorphologicalWatershedImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMorphologicalWatershedImage", "ITKWatersheds", "Watersheds"};
}

//------------------------------------------------------------------------------
Parameters ITKMorphologicalWatershedImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Level_Key, "Level", "", 0.0));
  params.insert(std::make_unique<BoolParameter>(k_MarkWatershedLine_Key, "MarkWatershedLine", "Set/Get whether the watershed pixel must be marked or not. Default is true. Set it to false do not only avoid writing watershed pixels, it also decrease algorithm complexity.", true));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn.", false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMorphologicalWatershedImage::clone() const
{
  return std::make_unique<ITKMorphologicalWatershedImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMorphologicalWatershedImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto level = filterArgs.value<float64>(k_Level_Key);
  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMorphologicalWatershedImage::ArrayOptionsType, cxITKMorphologicalWatershedImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMorphologicalWatershedImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  
  auto level = filterArgs.value<float64>(k_Level_Key);
  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKMorphologicalWatershedImage::ITKMorphologicalWatershedImageFunctor itkFunctor = {level, markWatershedLine, fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath); 
  
  return ITK::Execute<cxITKMorphologicalWatershedImage::ArrayOptionsType, cxITKMorphologicalWatershedImage::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
