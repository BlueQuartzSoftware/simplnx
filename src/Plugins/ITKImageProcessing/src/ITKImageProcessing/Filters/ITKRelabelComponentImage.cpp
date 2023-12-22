#include "ITKRelabelComponentImage.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkRelabelComponentImageFilter.h>

using namespace nx::core;

namespace cxITKRelabelComponentImage
{
using ArrayOptionsType = ITK::IntegerScalarPixelIdTypeList;

struct ITKRelabelComponentImageFunctor
{
  uint64 minimumObjectSize = 0u;
  bool sortByObjectSize = true;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::RelabelComponentImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetMinimumObjectSize(minimumObjectSize);
    filter->SetSortByObjectSize(sortByObjectSize);
    return filter;
  }
};
} // namespace cxITKRelabelComponentImage

namespace nx::core
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
  return {className(), "ITKImageProcessing", "ITKRelabelComponentImage", "ITKConnectedComponents", "ConnectedComponents"};
}

//------------------------------------------------------------------------------
Parameters ITKRelabelComponentImage::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<UInt64Parameter>(
      k_MinimumObjectSize_Key, "MinimumObjectSize",
      "Set the minimum size in pixels for an object. All objects smaller than this size will be discarded and will not appear in the output label map. NumberOfObjects will count only the objects "
      "whose pixel counts are greater than or equal to the minimum size. Call GetOriginalNumberOfObjects to find out how many objects were present in the original label map.",
      0u));
  params.insert(std::make_unique<BoolParameter>(k_SortByObjectSize_Key, "SortByObjectSize", "Controls whether the object labels are sorted by size. If false, initial order of labels is kept.", true));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedImageDataPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageDataPath_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

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
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKRelabelComponentImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKRelabelComponentImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_SelectedImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageDataPath_Key);
  const DataPath outputArrayPath = selectedInputArray.getParent().createChildPath(outputArrayName);

  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);

  const cxITKRelabelComponentImage::ITKRelabelComponentImageFunctor itkFunctor = {minimumObjectSize, sortByObjectSize};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);

  return ITK::Execute<cxITKRelabelComponentImage::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MinimumObjectSizeKey = "MinimumObjectSize";
constexpr StringLiteral k_SortByObjectSizeKey = "SortByObjectSize";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKRelabelComponentImage::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKRelabelComponentImage().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt64FilterParameterConverter>(args, json, SIMPL::k_MinimumObjectSizeKey, k_MinimumObjectSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SortByObjectSizeKey, k_SortByObjectSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_SelectedImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_SelectedImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageDataPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
