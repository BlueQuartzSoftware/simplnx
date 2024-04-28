#include "CreateImageGeometryFilter.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <sstream>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <string>

using namespace std::string_literals;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CreateImageGeometryFilter::name() const
{
  return FilterTraits<CreateImageGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateImageGeometryFilter::className() const
{
  return FilterTraits<CreateImageGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateImageGeometryFilter::uuid() const
{
  return FilterTraits<CreateImageGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateImageGeometryFilter::humanName() const
{
  return "Create Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateImageGeometryFilter::defaultTags() const
{
  return {className(), "Core", "Generation",
          "ImageGeometry"
          "Create Geometry"};
}

//------------------------------------------------------------------------------
Parameters CreateImageGeometryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_GeometryDataPath_Key, "Geometry Name [Data Group]", "The complete path to the Geometry being created", DataPath({"[Image Geometry]"})));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_Dimensions_Key, "Dimensions", "The number of cells in each of the X, Y, Z directions", std::vector<uint64_t>{20ULL, 60ULL, 200ULL},
                                                        std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of each of the axes in X, Y, Z order", std::vector<float32>(3), std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "The length scale of each voxel/pixel", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Cell Data Name", "The name of the cell Attribute Matrix to be created", ImageGeom::k_CellDataName));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateImageGeometryFilter::clone() const
{
  return std::make_unique<CreateImageGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateImageGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_GeometryDataPath_Key);
  auto pDimensionsValue = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);

  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::stringstream ss;
  ss << "Extents:\n"
     << "X Extent: 0 to " << pDimensionsValue[0] - 1 << " (dimension: " << pDimensionsValue[0] << ")\n"
     << "Y Extent: 0 to " << pDimensionsValue[1] - 1 << " (dimension: " << pDimensionsValue[1] << ")\n"
     << "Z Extent: 0 to " << pDimensionsValue[2] - 1 << " (dimension: " << pDimensionsValue[2] << ")\n"
     << "Bounds:\n"
     << "X Range: " << pOriginValue[0] << " to " << (pOriginValue[0] + static_cast<float>(pDimensionsValue[0]) * pSpacingValue[0])
     << " (delta: " << (static_cast<float>(pDimensionsValue[0]) * pSpacingValue[0]) << ")\n"
     << "Y Range: " << (pOriginValue[1]) << " to " << (pOriginValue[1] + static_cast<float>(pDimensionsValue[1]) * pSpacingValue[1])
     << " (delta: " << (static_cast<float>(pDimensionsValue[1]) * pSpacingValue[1]) << ")\n"
     << "Z Range: " << (pOriginValue[2]) << " to " << (pOriginValue[2] + static_cast<float>(pDimensionsValue[2]) * pSpacingValue[2])
     << " (delta: " << (static_cast<float>(pDimensionsValue[2]) * pSpacingValue[2]) << ")\n";
  std::string boxDimensions = ss.str();

  // Define a custom class that generates the changes to the DataStructure.
  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(
      pImageGeometryPath, CreateImageGeometryAction::DimensionType({pDimensionsValue[0], pDimensionsValue[1], pDimensionsValue[2]}), pOriginValue, pSpacingValue, cellDataName);

  // Assign the createImageGeometryAction to the Result<OutputActions>::actions vector via a push_back
  nx::core::Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  IFilter::PreflightResult preflightResult;
  // Assign/Move the resultOutputActions to the preflightResult object

  std::vector<PreflightValue> preflightUpdatedValues = {{"BoxDimensions", boxDimensions}};

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateImageGeometryFilter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_GeometryDataPath_Key);
  auto pDimensionsValue = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  return {};
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedDataContainerKey = "SelectedDataContainer";
constexpr StringLiteral k_DimensionsKey = "Dimensions";
constexpr StringLiteral k_OriginKey = "Origin";
constexpr StringLiteral k_SpacingKey = "Spacing";
constexpr StringLiteral k_BoxDimensionsKey = "BoxDimensions";
} // namespace SIMPL
} // namespace

Result<Arguments> CreateImageGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateImageGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedDataContainerKey, k_GeometryDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt64Vec3FilterParameterConverter>(args, json, SIMPL::k_DimensionsKey, k_Dimensions_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_OriginKey, k_Origin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_SpacingKey, k_Spacing_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
