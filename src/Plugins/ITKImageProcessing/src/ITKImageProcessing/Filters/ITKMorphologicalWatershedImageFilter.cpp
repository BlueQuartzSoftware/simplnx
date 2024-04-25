#include "ITKMorphologicalWatershedImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkMorphologicalWatershedImageFilter.h>

using namespace nx::core;

namespace cxITKMorphologicalWatershedImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
template <class PixelT>
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
} // namespace cxITKMorphologicalWatershedImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedImageFilter::name() const
{
  return FilterTraits<ITKMorphologicalWatershedImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedImageFilter::className() const
{
  return FilterTraits<ITKMorphologicalWatershedImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMorphologicalWatershedImageFilter::uuid() const
{
  return FilterTraits<ITKMorphologicalWatershedImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMorphologicalWatershedImageFilter::humanName() const
{
  return "ITK Morphological Watershed Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMorphologicalWatershedImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMorphologicalWatershedImage", "ITKWatersheds", "Watersheds"};
}

//------------------------------------------------------------------------------
Parameters ITKMorphologicalWatershedImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Level_Key, "Level", "", 0.0));
  params.insert(std::make_unique<BoolParameter>(
      k_MarkWatershedLine_Key, "MarkWatershedLine",
      "Set/Get whether the watershed pixel must be marked or not. Default is true. Set it to false do not only avoid writing watershed pixels, it also decrease algorithm complexity.", true));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected",
                                                "Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. "
                                                "For objects that are 1 pixel wide, use FullyConnectedOn.",
                                                false));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Array Name",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMorphologicalWatershedImageFilter::clone() const
{
  return std::make_unique<ITKMorphologicalWatershedImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMorphologicalWatershedImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto level = filterArgs.value<float64>(k_Level_Key);
  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions =
      ITK::DataCheck<cxITKMorphologicalWatershedImageFilter::ArrayOptionsType, cxITKMorphologicalWatershedImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMorphologicalWatershedImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto level = filterArgs.value<float64>(k_Level_Key);
  auto markWatershedLine = filterArgs.value<bool>(k_MarkWatershedLine_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKMorphologicalWatershedImageFilter::ITKMorphologicalWatershedImageFunctor itkFunctor = {level, markWatershedLine, fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKMorphologicalWatershedImageFilter::ArrayOptionsType, cxITKMorphologicalWatershedImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                              itkFunctor, shouldCancel);
}
} // namespace nx::core
