#include "IdentifySampleFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/IdentifySample.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{

//------------------------------------------------------------------------------
std::string IdentifySampleFilter::name() const
{
  return FilterTraits<IdentifySampleFilter>::name;
}

//------------------------------------------------------------------------------
std::string IdentifySampleFilter::className() const
{
  return FilterTraits<IdentifySampleFilter>::className;
}

//------------------------------------------------------------------------------
Uuid IdentifySampleFilter::uuid() const
{
  return FilterTraits<IdentifySampleFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string IdentifySampleFilter::humanName() const
{
  return "Isolate Largest Feature (Identify Sample)";
}

//------------------------------------------------------------------------------
std::vector<std::string> IdentifySampleFilter::defaultTags() const
{
  return {className(), "Core", "Identify Sample"};
}

//------------------------------------------------------------------------------
Parameters IdentifySampleFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_FillHoles_Key, "Fill Holes in Largest Feature", "Whether to fill holes within sample after it is identified", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SliceBySlice_Key, "Process Data Slice-By-Slice",
                                                                 "Whether to identify the largest sample (and optionally fill holes) slice-by-slice.  This option is useful if you have a sample that "
                                                                 "is not water-tight and the holes open up to the overscan section, or if you have holes that sit on a boundary.  The original "
                                                                 "algorithm will not fill holes that have these characteristics, only holes that are completely enclosed by the sample and "
                                                                 "water-tight.  If you have holes that are not water-tight or sit on a boundary, choose this option and then pick the plane that will "
                                                                 "allow the holes to be water-tight on each slice of that plane.",
                                                                 false));
  params.insert(
      std::make_unique<ChoicesParameter>(k_SliceBySlicePlane_Key, "Slice-By-Slice Plane",
                                         "Set the plane that the data will be processed slice-by-slice.  For example, if you pick the XY plane, the data will be processed in the Z direction.", 0,
                                         ChoicesParameter::Choices{"XY", "XZ", "YZ"}));

  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Image Geometry", "DataPath to the target ImageGeom", DataPath(),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the mask array defining what is sample and what is not", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::boolean, nx::core::DataType::uint8},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.linkParameters(k_SliceBySlice_Key, k_SliceBySlicePlane_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType IdentifySampleFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer IdentifySampleFilter::clone() const
{
  return std::make_unique<IdentifySampleFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult IdentifySampleFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto goodVoxelsArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  nx::core::MarkDataPathModified(dataStructure, resultOutputActions, goodVoxelsArrayPath);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> IdentifySampleFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  IdentifySampleInputValues inputValues;
  inputValues.FillHoles = filterArgs.value<bool>(k_FillHoles_Key);
  inputValues.SliceBySlice = filterArgs.value<bool>(k_SliceBySlice_Key);
  inputValues.SliceBySlicePlaneIndex = filterArgs.value<ChoicesParameter::ValueType>(k_SliceBySlicePlane_Key);
  inputValues.InputImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  return IdentifySample(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FillHolesKey = "FillHoles";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> IdentifySampleFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = IdentifySampleFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FillHolesKey, k_FillHoles_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
