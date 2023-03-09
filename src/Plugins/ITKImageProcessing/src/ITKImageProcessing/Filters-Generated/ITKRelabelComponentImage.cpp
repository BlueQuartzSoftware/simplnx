#include "ITKRelabelComponentImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <itkRelabelComponentImageFilter.h>

using namespace complex;

namespace cxITKRelabelComponentImage
{
using ArrayOptionsT = ITK::IntegerScalarPixelIdTypeList;

struct ITKRelabelComponentImageFunctor
{
  uint64 minimumObjectSize = 0u;
  bool sortByObjectSize = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterT = itk::RelabelComponentImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterT::New();
    filter->SetMinimumObjectSize(minimumObjectSize);
    filter->SetSortByObjectSize(sortByObjectSize);
    return filter;
  }
};
} // namespace cxITKRelabelComponentImage

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKRelabelComponentImage::name() const
{
  return FilterTraits<ITKRelabelComponentImage>::name;
}

//------------------------------------------------------------------------------
std::string ITKRelabelComponentImage::className() const
{
  return FilterTraits<ITKRelabelComponentImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKRelabelComponentImage::uuid() const
{
  return FilterTraits<ITKRelabelComponentImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKRelabelComponentImage::humanName() const
{
  return "ITK Relabel Component Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKRelabelComponentImage::defaultTags() const
{
  return {"ITKImageProcessing", "ITKRelabelComponentImage", "ITKConnectedComponents", "ConnectedComponents"};
}

//------------------------------------------------------------------------------
Parameters ITKRelabelComponentImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<UInt64Parameter>(k_MinimumObjectSize_Key, "MinimumObjectSize", "", 0u));
  params.insert(std::make_unique<BoolParameter>(k_SortByObjectSize_Key, "SortByObjectSize", "", true));

  params.insertSeparator(Parameters::Separator{"Input Data Structure Items"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          complex::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKRelabelComponentImage::clone() const
{
  return std::make_unique<ITKRelabelComponentImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKRelabelComponentImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);
  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKRelabelComponentImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKRelabelComponentImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayPath = filterArgs.value<DataPath>(k_OutputImageDataPath_Key);

  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);

  cxITKRelabelComponentImage::ITKRelabelComponentImageFunctor itkFunctor = {minimumObjectSize, sortByObjectSize};

  // LINK GEOMETRY OUTPUT START
  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);
  // LINK GEOMETRY OUTPUT STOP

  return ITK::Execute<cxITKRelabelComponentImage::ArrayOptionsT>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, messageHandler);
}
} // namespace complex
