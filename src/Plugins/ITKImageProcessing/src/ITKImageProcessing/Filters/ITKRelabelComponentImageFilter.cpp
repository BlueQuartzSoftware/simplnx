#include "ITKRelabelComponentImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkRelabelComponentImageFilter.h>

using namespace nx::core;

namespace cxITKRelabelComponentImageFilter
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
} // namespace cxITKRelabelComponentImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKRelabelComponentImageFilter::name() const
{
  return FilterTraits<ITKRelabelComponentImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKRelabelComponentImageFilter::className() const
{
  return FilterTraits<ITKRelabelComponentImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKRelabelComponentImageFilter::uuid() const
{
  return FilterTraits<ITKRelabelComponentImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKRelabelComponentImageFilter::humanName() const
{
  return "ITK Relabel Component Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKRelabelComponentImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKRelabelComponentImage", "ITKConnectedComponents", "ConnectedComponents"};
}

//------------------------------------------------------------------------------
Parameters ITKRelabelComponentImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt64Parameter>(
      k_MinimumObjectSize_Key, "Minimum Object Size",
      "Set the minimum size in pixels for an object. All objects smaller than this size will be discarded and will not appear in the output label map. NumberOfObjects will count only the objects "
      "whose pixel counts are greater than or equal to the minimum size. Call GetOriginalNumberOfObjects to find out how many objects were present in the original label map.",
      0u));
  params.insert(std::make_unique<BoolParameter>(k_SortByObjectSize_Key, "SortByObjectSize", "Controls whether the object labels are sorted by size. If false, initial order of labels is kept.", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetIntegerScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKRelabelComponentImageFilter::clone() const
{
  return std::make_unique<ITKRelabelComponentImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKRelabelComponentImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKRelabelComponentImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKRelabelComponentImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);

  const cxITKRelabelComponentImageFilter::ITKRelabelComponentImageFunctor itkFunctor = {minimumObjectSize, sortByObjectSize};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKRelabelComponentImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
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

Result<Arguments> ITKRelabelComponentImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKRelabelComponentImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt64FilterParameterConverter>(args, json, SIMPL::k_MinimumObjectSizeKey, k_MinimumObjectSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SortByObjectSizeKey, k_SortByObjectSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
