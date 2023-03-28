#include "AppendImageGeometryZSliceFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AppendImageGeometryZSlice.hpp"
#include "FindArrayStatisticsFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AppendImageGeometryZSliceFilter::name() const
{
  return FilterTraits<AppendImageGeometryZSliceFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AppendImageGeometryZSliceFilter::className() const
{
  return FilterTraits<AppendImageGeometryZSliceFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AppendImageGeometryZSliceFilter::uuid() const
{
  return FilterTraits<AppendImageGeometryZSliceFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AppendImageGeometryZSliceFilter::humanName() const
{
  return "Append Z Slice (Image Geometry)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AppendImageGeometryZSliceFilter::defaultTags() const
{
  return {"Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters AppendImageGeometryZSliceFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputGeometry_Key, "Input Image Geometry", "The incoming image geometry (cell data) that is to be appended.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_DestinationGeometry_Key, "Destination Image Geometry",
                                                             "The destination image geometry (cell data) that is the final location for the appended data.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<BoolParameter>(k_CheckResolution_Key, "Check Spacing", "Checks to make sure the spacing for the input geometry and destination geometry match", false));
  // params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewGeometry_Key, "Save as new geometry",
  //                                                                "Save the combined data as a new geometry instead of appending the input data to the destination geometry", false));
  // params.insert(std::make_unique<DataGroupCreationParameter>(k_NewGeometry_Key, "New Image Geometry Name", "The path to the new geometry with the combined data from the input & destination
  // geometry",
  //                                                            DataPath({"AppendedImageGeom"})));
  // params.linkParameters(k_SaveAsNewGeometry_Key, k_NewGeometry_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AppendImageGeometryZSliceFilter::clone() const
{
  return std::make_unique<AppendImageGeometryZSliceFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AppendImageGeometryZSliceFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto pInputGeometryPathValue = filterArgs.value<DataPath>(k_InputGeometry_Key);
  auto pDestinationGeometryPathValue = filterArgs.value<DataPath>(k_DestinationGeometry_Key);
  auto pCheckResolutionValue = filterArgs.value<bool>(k_CheckResolution_Key);
  // auto pSaveAsNewGeometry = filterArgs.value<bool>(k_SaveAsNewGeometry_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& inputGeometry = dataStructure.getDataRefAs<ImageGeom>(pInputGeometryPathValue);
  const auto& destGeometry = dataStructure.getDataRefAs<ImageGeom>(pDestinationGeometryPathValue);
  SizeVec3 inputGeomDims = inputGeometry.getDimensions();
  SizeVec3 destGeomDims = destGeometry.getDimensions();
  if(destGeomDims[0] != inputGeomDims[0])
  {
    return MakePreflightErrorResult(-8200, fmt::format("Input X Dim ({}) not equal to Destination X Dim ({})", inputGeomDims[0], destGeomDims[0]));
  }
  if(destGeomDims[0] != inputGeomDims[0])
  {
    return MakePreflightErrorResult(-8201, fmt::format("Input Y Dim ({}) not equal to Destination Y Dim ({})", inputGeomDims[1], destGeomDims[1]));
  }
  if(pCheckResolutionValue)
  {
    FloatVec3 inputRes = inputGeometry.getSpacing();
    FloatVec3 destRes = destGeometry.getSpacing();

    if(inputRes[0] != destRes[0])
    {
      return MakePreflightErrorResult(-8202, fmt::format("Input X Spacing ({}) not equal to Destination X Spacing ({})", inputRes[0], destRes[0]));
    }
    if(inputRes[1] != destRes[1])
    {
      return MakePreflightErrorResult(-8203, fmt::format("Input Y Spacing ({}) not equal to Destination Y Spacing ({})", inputRes[1], destRes[1]));
    }
    if(inputRes[2] != destRes[2])
    {
      return MakePreflightErrorResult(-8204, fmt::format("Input Z Spacing ({}) not equal to Destination Z Spacing ({})", inputRes[2], destRes[2]));
    }
  }

  std::vector<usize> newDims = {destGeomDims[0], destGeomDims[1], inputGeomDims[2] + destGeomDims[2]};
  FloatVec3 origin = destGeometry.getOrigin();
  FloatVec3 spacing = destGeometry.getSpacing();
  // if(pSaveAsNewGeometry)
  //{
  //   auto pNewImageGeomPath = filterArgs.value<DataPath>(k_NewGeometry_Key);
  //   auto createGeomAction = std::make_unique<CreateImageGeometryAction>(pNewImageGeomPath, newDims, std::vector<float>{origin[0], origin[1], origin[2]},
  //                                                                       std::vector<float>{spacing[0], spacing[1], spacing[2]}, ImageGeom::k_CellDataName);
  //
  //   std::vector<usize> newCellDataDims(newDims.rbegin(), newDims.rend());
  //
  //   resultOutputActions.value().actions.push_back(std::move(createGeomAction));
  // }

  resultOutputActions.warnings().push_back(
      {-8405, "You are appending cell data together which may change the number of features. As a result, any feature level attribute matrix data will likely be invalidated!"});
  preflightUpdatedValues.push_back({"Appended Image Geometry Info", complex::GeometryHelpers::Description::GenerateGeometryInfo(newDims, spacing, origin)});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AppendImageGeometryZSliceFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  AppendImageGeometryZSliceInputValues inputValues;

  inputValues.InputGeometryPath = filterArgs.value<DataPath>(k_InputGeometry_Key);
  inputValues.DestinationGeometryPath = filterArgs.value<DataPath>(k_DestinationGeometry_Key);
  inputValues.CheckResolution = filterArgs.value<bool>(k_CheckResolution_Key);
  // inputValues.SaveAsNewGeometry = filterArgs.value<bool>(k_SaveAsNewGeometry_Key);

  return AppendImageGeometryZSlice(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
