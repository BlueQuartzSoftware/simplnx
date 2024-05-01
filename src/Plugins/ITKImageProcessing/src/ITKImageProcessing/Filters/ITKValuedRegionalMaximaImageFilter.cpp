#include "ITKValuedRegionalMaximaImageFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/sitkCommon.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <itkValuedRegionalMaximaImageFilter.h>

using namespace nx::core;

namespace cxITKValuedRegionalMaximaImageFilter
{
using ArrayOptionsType = ITK::ScalarPixelIdTypeList;

struct ITKValuedRegionalMaximaImageFunctor
{
  bool fullyConnected = false;

  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {
    using FilterType = itk::ValuedRegionalMaximaImageFilter<InputImageT, OutputImageT>;
    auto filter = FilterType::New();
    filter->SetFullyConnected(fullyConnected);
    return filter;
  }
};
} // namespace cxITKValuedRegionalMaximaImageFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImageFilter::name() const
{
  return FilterTraits<ITKValuedRegionalMaximaImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImageFilter::className() const
{
  return FilterTraits<ITKValuedRegionalMaximaImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKValuedRegionalMaximaImageFilter::uuid() const
{
  return FilterTraits<ITKValuedRegionalMaximaImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKValuedRegionalMaximaImageFilter::humanName() const
{
  return "ITK Valued Regional Maxima Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKValuedRegionalMaximaImageFilter::defaultTags() const
{
  return {className(), "ITKImageProcessing", "ITKValuedRegionalMaximaImage", "ITKMathematicalMorphology", "MathematicalMorphology"};
}

//------------------------------------------------------------------------------
Parameters ITKValuedRegionalMaximaImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected Components",
                                                "Whether the connected components are defined strictly by face connectivity (False) or by face+edge+vertex connectivity (True). Default is False"
                                                "For objects that are 1 pixel wide, use True.",
                                                false));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKValuedRegionalMaximaImageFilter::clone() const
{
  return std::make_unique<ITKValuedRegionalMaximaImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKValuedRegionalMaximaImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ITK::DataCheck<cxITKValuedRegionalMaximaImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKValuedRegionalMaximaImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  const cxITKValuedRegionalMaximaImageFilter::ITKValuedRegionalMaximaImageFunctor itkFunctor = {fullyConnected};

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  return ITK::Execute<cxITKValuedRegionalMaximaImageFilter::ArrayOptionsType>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKValuedRegionalMaximaImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKValuedRegionalMaximaImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
