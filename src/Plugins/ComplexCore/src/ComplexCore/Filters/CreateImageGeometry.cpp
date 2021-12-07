#include "CreateImageGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <sstream>
#include <string>

using namespace std::string_literals;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CreateImageGeometry::name() const
{
  return FilterTraits<CreateImageGeometry>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateImageGeometry::className() const
{
  return FilterTraits<CreateImageGeometry>::className;
}

//------------------------------------------------------------------------------
Uuid CreateImageGeometry::uuid() const
{
  return FilterTraits<CreateImageGeometry>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateImageGeometry::humanName() const
{
  return "Create Geometry (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateImageGeometry::defaultTags() const
{
  return {"#Core", "#Generation",
          "#ImageGeometry"
          "#Create Geometry"};
}

//------------------------------------------------------------------------------
Parameters CreateImageGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedDataGroup_Key, "Data Group Destination", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_GeometryName_Key, "Name of Geometry", "", std::string("Image Geometry")));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_Dimensions_Key, "Dimensions", "", std::vector<uint64_t>{20ULL, 60ULL, 200ULL}, std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X"s, "Y"s, "Z"s}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateImageGeometry::clone() const
{
  return std::make_unique<CreateImageGeometry>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateImageGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedDataGroupValue = filterArgs.value<DataPath>(k_SelectedDataGroup_Key);
  auto pGeometryNameValue = filterArgs.value<std::string>(k_GeometryName_Key);
  auto pDimensionsValue = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);

  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::stringstream ss;
  std::array<float, 3> halfRes = {pSpacingValue[0] * 0.5F, pSpacingValue[1] * 0.5F, pSpacingValue[2] * 0.5F};
  ss << "Extents:\n"
     << "X Extent: 0 to " << pDimensionsValue[0] - 1 << " (dimension: " << pDimensionsValue[0] << ")\n"
     << "Y Extent: 0 to " << pDimensionsValue[1] - 1 << " (dimension: " << pDimensionsValue[1] << ")\n"
     << "Z Extent: 0 to " << pDimensionsValue[2] - 1 << " (dimension: " << pDimensionsValue[2] << ")\n"
     << "Bounds:\n"
     << "X Range: " << (pOriginValue[0] - halfRes[0]) << " to " << (pOriginValue[0] - halfRes[0] + static_cast<float>(pDimensionsValue[0]) * pSpacingValue[0])
     << " (delta: " << (static_cast<float>(pDimensionsValue[0]) * pSpacingValue[0]) << ")\n"
     << "Y Range: " << (pOriginValue[1] - halfRes[1]) << " to " << (pOriginValue[1] - halfRes[1] + static_cast<float>(pDimensionsValue[1]) * pSpacingValue[1])
     << " (delta: " << (static_cast<float>(pDimensionsValue[1]) * pSpacingValue[1]) << ")\n"
     << "Z Range: " << (pOriginValue[2] - halfRes[2]) << " to " << (pOriginValue[2] - halfRes[2] + static_cast<float>(pDimensionsValue[2]) * pSpacingValue[2])
     << " (delta: " << (static_cast<float>(pDimensionsValue[2]) * pSpacingValue[2]) << ")\n";
  std::string boxDimensions = ss.str();

  DataPath fullPath = pSelectedDataGroupValue.createChildPath(pGeometryNameValue);

  // Define a custom class that generates the changes to the DataStructure.
  auto createImageGeometryAction =
      std::make_unique<CreateImageGeometryAction>(fullPath, CreateImageGeometryAction::DimensionType({pDimensionsValue[0], pDimensionsValue[1], pDimensionsValue[2]}), pOriginValue, pSpacingValue);

  // Assign the createImageGeometryAction to the Result<OutputActions>::actions vector via a push_back
  complex::Result<OutputActions> resultOutputActions;
  resultOutputActions.value().actions.push_back(std::move(createImageGeometryAction));

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  IFilter::PreflightResult preflightResult;
  // Assign/Move the resultOutputActions to the preflightResult object

  std::vector<PreflightValue> preflightUpdatedValues = {{"BoxDimensions", boxDimensions}};

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateImageGeometry::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedDataGroupValue = filterArgs.value<DataPath>(k_SelectedDataGroup_Key);
  auto pGeometryNameValue = filterArgs.value<std::string>(k_GeometryName_Key);
  auto pDimensionsValue = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  return {};
}
} // namespace complex
