#include "ITKSignedDanielssonDistanceMapImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include <itkSignedDanielssonDistanceMapImageFilter.h>

using namespace nx::core;

namespace cxITKSignedDanielssonDistanceMapImageFilter
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;
template <class PixelT>
using FilterOutputType = float32;

struct ITKSignedDanielssonDistanceMapImageFilterFunctor
{
  bool insideIsPositive = false;
  bool squaredDistance = false;
  bool useImageSpacing = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::SignedDanielssonDistanceMapImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetInsideIsPositive(insideIsPositive);
    filter->SetSquaredDistance(squaredDistance);
    filter->SetUseImageSpacing(useImageSpacing);
    return filter;
  }
};
} // namespace cxITKSignedDanielssonDistanceMapImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKSignedDanielssonDistanceMapImageFilter::name() const
{
  return FilterTraits<ITKSignedDanielssonDistanceMapImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKSignedDanielssonDistanceMapImageFilter::className() const
{
  return FilterTraits<ITKSignedDanielssonDistanceMapImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKSignedDanielssonDistanceMapImageFilter::uuid() const
{
  return FilterTraits<ITKSignedDanielssonDistanceMapImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKSignedDanielssonDistanceMapImageFilter::humanName() const
{
  return "ITK Signed Danielsson Distance Map Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKSignedDanielssonDistanceMapImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKSignedDanielssonDistanceMapImageFilter", "ITKDistanceMap", "DistanceMap"};
}

//------------------------------------------------------------------------------
Parameters ITKSignedDanielssonDistanceMapImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_InsideIsPositive_Key, "Inside Is Positive",
                                                "Set if the inside represents positive values in the signed distance map. By convention ON pixels are treated as inside pixels.", false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "Squared Distance", "Set if the distance should be squared.", false));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "Use Image Spacing", "Set if image spacing should be used in computing distances.", false));

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
IFilter::UniquePointer ITKSignedDanielssonDistanceMapImageFilter::clone() const
{
  return std::make_unique<ITKSignedDanielssonDistanceMapImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKSignedDanielssonDistanceMapImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                  const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKSignedDanielssonDistanceMapImageFilter::ArrayOptionsType, cxITKSignedDanielssonDistanceMapImageFilter::FilterOutputType>(
      dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKSignedDanielssonDistanceMapImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto insideIsPositive = filterArgs.value<bool>(k_InsideIsPositive_Key);
  auto squaredDistance = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto useImageSpacing = filterArgs.value<bool>(k_UseImageSpacing_Key);

  const cxITKSignedDanielssonDistanceMapImageFilter::ITKSignedDanielssonDistanceMapImageFilterFunctor itkFunctor = {insideIsPositive, squaredDistance, useImageSpacing};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKSignedDanielssonDistanceMapImageFilter::ArrayOptionsType, cxITKSignedDanielssonDistanceMapImageFilter::FilterOutputType>(dataStructure, selectedInputArray, imageGeomPath,
                                                                                                                                                    outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
