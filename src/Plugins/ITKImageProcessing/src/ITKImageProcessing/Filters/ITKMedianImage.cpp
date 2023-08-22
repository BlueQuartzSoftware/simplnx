#include "ITKMedianImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"


#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkMedianImageFilter.h>

using namespace complex;

namespace cxITKMedianImage
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;
  //VectorPixelIDTypeList;

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
} // namespace cxITKMedianImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMedianImage::name() const
{
  return FilterTraits<ITKMedianImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKMedianImage::className() const
{
  return FilterTraits<ITKMedianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMedianImage::uuid() const
{
  return FilterTraits<ITKMedianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMedianImage::humanName() const
{
  return "ITK Median Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMedianImage::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKMedianImage", "ITKSmoothing", "Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKMedianImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorUInt32Parameter>(k_Radius_Key, "Radius", "Radius Dimensions XYZ", std::vector<unsigned int>(3, 1), std::vector<std::string>{"X", "Y", "Z"}));


  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{}, complex::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMedianImage::clone() const
{
  return std::make_unique<ITKMedianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMedianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto radius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);

  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKMedianImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKMedianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  
  auto radius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);


  const cxITKMedianImage::ITKMedianImageFunctor itkFunctor = {radius};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath); 
  
  return ITK::Execute<cxITKMedianImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}
} // namespace complex
