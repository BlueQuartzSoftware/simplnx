#include "ITKDanielssonDistanceMapImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include <itkDanielssonDistanceMapImageFilter.h>

using namespace nx::core;

namespace cxITKDanielssonDistanceMapImageFilter
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKDanielssonDistanceMapImageFilterFunctor
{
  bool inputIsBinary = false;
  bool squaredDistance = false;
  bool useImageSpacing = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::DanielssonDistanceMapImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetInputIsBinary(inputIsBinary);
    filter->SetSquaredDistance(squaredDistance);
    filter->SetUseImageSpacing(useImageSpacing);
    return filter;
  }
};
} // namespace cxITKDanielssonDistanceMapImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKDanielssonDistanceMapImageFilter::name() const
{
  return FilterTraits<ITKDanielssonDistanceMapImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKDanielssonDistanceMapImageFilter::className() const
{
  return FilterTraits<ITKDanielssonDistanceMapImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKDanielssonDistanceMapImageFilter::uuid() const
{
  return FilterTraits<ITKDanielssonDistanceMapImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKDanielssonDistanceMapImageFilter::humanName() const
{
  return "ITK Danielsson Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKDanielssonDistanceMapImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKDanielssonDistanceMapImageFilter", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKDanielssonDistanceMapImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(
      std::make_unique<BoolParameter>(k_InputIsBinary_Key, "Input Is Binary",
                                      "Set/Get if the input is binary. If this variable is set, each nonzero pixel in the input image will be given a unique numeric code to be used by the Voronoi "
                                      "partition. If the image is binary but you are not interested in the Voronoi regions of the different nonzero pixels, then you need not set this.",
                                      false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "Squared Distance", "Set/Get if the distance should be squared.", false));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "Use Image Spacing", "Set/Get if image spacing should be used in computing distances.", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKDanielssonDistanceMapImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKDanielssonDistanceMapImageFilter::clone() const
{
  return std::make_unique<ITKDanielssonDistanceMapImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKDanielssonDistanceMapImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto inputIsBinary = filterArgs.value<bool>(k_InputIsBinary_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKDanielssonDistanceMapImageFilter::ArrayOptionsType, cxITKDanielssonDistanceMapImageFilter::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKDanielssonDistanceMapImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto inputIsBinary = filterArgs.value<bool>(k_InputIsBinary_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  const cxITKDanielssonDistanceMapImageFilter::ITKDanielssonDistanceMapImageFilterFunctor itkFunctor = {inputIsBinary, squaredDistance, useImageSpacing};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKDanielssonDistanceMapImageFilter::ArrayOptionsType, cxITKDanielssonDistanceMapImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                        outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
