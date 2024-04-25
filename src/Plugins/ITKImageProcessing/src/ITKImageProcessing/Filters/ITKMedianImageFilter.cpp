#include "ITKMedianImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkMedianImageFilter.h>

using namespace nx::core;

namespace cxITKMedianImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
// VectorPixelIDTypeList;

struct ITKMedianImageFunctor
{
  using RadiusInputRadiusType = std::vector<uint32>;
  RadiusInputRadiusType radius = std::vector<unsigned int>(3, 1);

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::MedianImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    // Set the Radius Filter Property
    {
      using RadiusType = typename FilterType::RadiusType;
      auto convertedRadius = ITK::CastVec3ToITK<RadiusType, typename RadiusType::SizeValueType>(radius, RadiusType::Dimension);
      filter->SetRadius(convertedRadius);
    }

    return filter;
  }
};
} // namespace cxITKMedianImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMedianImageFilter::name() const
{
  return FilterTraits<ITKMedianImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKMedianImageFilter::className() const
{
  return FilterTraits<ITKMedianImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMedianImageFilter::uuid() const
{
  return FilterTraits<ITKMedianImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMedianImageFilter::humanName() const
{
  return "ITK Median Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMedianImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMedianImage", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKMedianImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorUInt32Parameter>(k_Radius_Key, "Radius", "Radius Dimensions XYZ", std::vector<unsigned int>(3, 1), std::vector<std::string>{"X", "Y", "Z"}));

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
IFilter::UniquePointer ITKMedianImageFilter::clone() const
{
  return std::make_unique<ITKMedianImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMedianImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto radius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMedianImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMedianImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto radius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  const cxITKMedianImageFilter::ITKMedianImageFunctor itkFunctor = {radius};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKMedianImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace nx::core
