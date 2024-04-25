#include "ITKHConvexImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkHConvexImageFilter.h>

using namespace nx::core;

namespace cxITKHConvexImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKHConvexImageFunctor
{
  float64 height = 2.0;
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::HConvexImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetHeight(height);
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKHConvexImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKHConvexImageFilter::name() const
{
  return FilterTraits<ITKHConvexImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKHConvexImageFilter::className() const
{
  return FilterTraits<ITKHConvexImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKHConvexImageFilter::uuid() const
{
  return FilterTraits<ITKHConvexImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKHConvexImageFilter::humanName() const
{
  return "ITK H Convex Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKHConvexImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKHConvexImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKHConvexImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float64Parameter>(k_Height_Key, "Height",
                                                   "Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below "
                                                   "this value are replaced with an estimate of the local background.",
                                                   2.0));
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
IFilter::UniquePointer ITKHConvexImageFilter::clone() const
{
  return std::make_unique<ITKHConvexImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKHConvexImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKHConvexImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKHConvexImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto height = filterArgs.value<float64>(k_Height_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKHConvexImageFilter::ITKHConvexImageFunctor itkFunctor = {height, fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKHConvexImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
