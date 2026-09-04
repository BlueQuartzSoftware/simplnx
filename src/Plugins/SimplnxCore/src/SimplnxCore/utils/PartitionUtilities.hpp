#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"

#include <fmt/format.h>

#include <optional>
#include <string_view>

namespace nx::core::PartitionUtilities
{

enum class PartitioningMode
{
  Basic = 0,
  Advanced = 1,
  BoundingBox = 2,
  ExistingPartitionGrid = 3
};

struct PSGeomInfo
{
  USizeVec3 geometryDims;
  std::optional<FloatVec3> geometryOrigin;
  std::optional<FloatVec3> geometrySpacing;
  IGeometry::LengthUnit geometryUnits;
};

namespace Parameters
{
inline constexpr StringLiteral k_PartitioningMode_Key = "partitioning_mode_index";
inline constexpr StringLiteral k_NumberOfCellsPerAxis_Key = "number_of_partitions_per_axis";
inline constexpr StringLiteral k_PartitionGridOrigin_Key = "partitioning_scheme_origin";
inline constexpr StringLiteral k_CellLength_Key = "length_per_partition";
inline constexpr StringLiteral k_MinGridCoord_Key = "lower_left_coord";
inline constexpr StringLiteral k_MaxGridCoord_Key = "upper_right_coord";
inline constexpr StringLiteral k_ExistingPartitionGridPath_Key = "existing_partitioning_scheme_path";

inline constexpr ChoicesParameter::ValueType k_BasicModeIndex = 0;
inline constexpr ChoicesParameter::ValueType k_AdvancedModeIndex = 1;
inline constexpr ChoicesParameter::ValueType k_BoundingBoxModeIndex = 2;
inline constexpr ChoicesParameter::ValueType k_ExistingSchemeModeIndex = 3;

inline const ChoicesParameter::Choices k_Choices = {"Basic (0)", "Advanced (1)", "Bounding Box (2)", "Existing Partition Grid (3)"};
} // namespace Parameters

// Inserts the shared partition grid parameters and their mode-based linkage into params.
// Call from each filter's parameters() before inserting filter-specific params.
inline void AppendPartitioningParameters(nx::core::Parameters& params)
{
  using namespace Parameters;
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_PartitioningMode_Key, "Select the partitioning mode",
                                                                    "Mode can be 'Basic (0)', 'Advanced (1)', 'Bounding Box (2)', 'Existing Partition Grid (3)'", 0, k_Choices));
  params.insert(std::make_unique<VectorInt32Parameter>(k_NumberOfCellsPerAxis_Key, "Number Of Cells Per Axis", "The number of cells along each axis of the partition grid",
                                                       std::vector<int32>({5, 5, 5}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_PartitionGridOrigin_Key, "Partition Grid Origin", "The origin of the generated partition geometry",
                                                         std::vector<float32>({0.0F, 0.0F, 0.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_CellLength_Key, "Cell Length (Physical Units)",
                                                         "The length in physical units for each cell in the partition grid. The physical units are automatically set by the input geometry.",
                                                         std::vector<float32>({1.0F, 1.0F, 1.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MinGridCoord_Key, "Minimum Grid Coordinate", "Minimum grid coordinate used to create the partition grid",
                                                         std::vector<float32>({0.0F, 0.0F, 0.0F}), std::vector<std::string>({"X", "Y", "Z"})));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_MaxGridCoord_Key, "Maximum Grid Coordinate", "Maximum grid coordinate used to create the partition grid",
                                                         std::vector<float32>({10.0F, 10.0F, 10.0F}), std::vector<std::string>({"X", "Y", "Z"})));

  params.linkParameters(k_PartitioningMode_Key, k_NumberOfCellsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_BasicModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfCellsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_NumberOfCellsPerAxis_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_PartitionGridOrigin_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_CellLength_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_MinGridCoord_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
  params.linkParameters(k_PartitioningMode_Key, k_MaxGridCoord_Key, std::make_any<ChoicesParameter::ValueType>(k_BoundingBoxModeIndex));
}

inline Result<> DataCheckNumberOfPartitions(const SizeVec3& numberOfPartitionsPerAxis)
{
  if(numberOfPartitionsPerAxis.getX() <= 0)
  {
    return MakeErrorResult(-3012, fmt::format("Number of Partitions Per Axis: The X dimension ({}) must be greater than 0.", numberOfPartitionsPerAxis.getX()));
  }
  if(numberOfPartitionsPerAxis.getY() <= 0)
  {
    return MakeErrorResult(-3013, fmt::format("Number of Partitions Per Axis: The Y dimension ({}) must be greater than 0.", numberOfPartitionsPerAxis.getY()));
  }
  if(numberOfPartitionsPerAxis.getZ() <= 0)
  {
    return MakeErrorResult(-3014, fmt::format("Number of Partitions Per Axis: The Z dimension ({}) must be greater than 0.", numberOfPartitionsPerAxis.getZ()));
  }
  return {};
}

inline Result<> DataCheckCellLength(std::string_view filterName, const FloatVec3& lengthPerPartition)
{
  if(lengthPerPartition.getX() < 0)
  {
    return MakeErrorResult(-3003, fmt::format("{}: Length Per Partition - The X value ({}) cannot be negative.", filterName, lengthPerPartition.getX()));
  }
  if(lengthPerPartition.getY() < 0)
  {
    return MakeErrorResult(-3004, fmt::format("{}: Length Per Partition - The Y value ({}) cannot be negative.", filterName, lengthPerPartition.getY()));
  }
  if(lengthPerPartition.getZ() < 0)
  {
    return MakeErrorResult(-3005, fmt::format("{}: Length Per Partition - The Z value ({}) cannot be negative.", filterName, lengthPerPartition.getZ()));
  }
  return {};
}

inline Result<> DataCheckBoundingBoxCoords(std::string_view filterName, const FloatVec3& llCoord, const FloatVec3& urCoord)
{
  if(llCoord.getX() > urCoord.getX())
  {
    return MakeErrorResult(-3006, fmt::format("{}: Lower Left Coordinate - X value ({}) is larger than the upper right coordinate X value ({}).", filterName, llCoord.getX(), urCoord.getX()));
  }
  if(llCoord.getY() > urCoord.getY())
  {
    return MakeErrorResult(-3007, fmt::format("{}: Lower Left Coordinate - Y value ({}) is larger than the upper right coordinate Y value ({}).", filterName, llCoord.getY(), urCoord.getY()));
  }
  if(llCoord.getZ() > urCoord.getZ())
  {
    return MakeErrorResult(-3008, fmt::format("{}: Lower Left Coordinate - Z value ({}) is larger than the upper right coordinate Z value ({}).", filterName, llCoord.getZ(), urCoord.getZ()));
  }
  return {};
}

// Non-template overload for all node-based geometries (Vertex, Edge, Triangle, Quad, Tetrahedral, Hexahedral).
inline Result<PSGeomInfo> GeneratePartitioningSchemeInfo(const INodeGeometry0D& geometry, const DataStructure& dataStructure, const Arguments& filterArgs)
{
  using namespace Parameters;
  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(k_PartitioningMode_Key);
  auto pNumberOfCellsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfCellsPerAxis_Key);
  const SizeVec3 numOfPartitionsPerAxisValue = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]),
                                                static_cast<usize>(pNumberOfCellsPerAxisValue[2])};
  PSGeomInfo psGeomMetadata;

  switch(static_cast<PartitioningMode>(pPartitioningModeValue))
  {
  case PartitioningMode::Basic: {
    Result<FloatVec3> originResult = GeometryUtilities::CalculateNodeBasedPartitionSchemeOrigin(geometry);
    Result<FloatVec3> pLengthResult = GeometryUtilities::CalculatePartitionLengthsByPartitionCount(geometry, numOfPartitionsPerAxisValue);
    if(originResult.valid() && pLengthResult.valid())
    {
      psGeomMetadata.geometryDims = numOfPartitionsPerAxisValue;
      psGeomMetadata.geometryOrigin = originResult.value();
      psGeomMetadata.geometrySpacing = pLengthResult.value();
      psGeomMetadata.geometryUnits = geometry.getUnits();
    }
    break;
  }
  case PartitioningMode::Advanced: {
    auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PartitionGridOrigin_Key);
    auto pCellLengthValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellLength_Key);
    psGeomMetadata.geometryDims = numOfPartitionsPerAxisValue;
    psGeomMetadata.geometryOrigin = pOriginValue;
    psGeomMetadata.geometrySpacing = pCellLengthValue;
    psGeomMetadata.geometryUnits = geometry.getUnits();
    break;
  }
  case PartitioningMode::BoundingBox: {
    auto pMinGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MinGridCoord_Key);
    auto pMaxGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaxGridCoord_Key);
    const FloatVec3 llCoord(pMinGridCoordValue[0], pMinGridCoordValue[1], pMinGridCoordValue[2]);
    const FloatVec3 urCoord(pMaxGridCoordValue[0], pMaxGridCoordValue[1], pMaxGridCoordValue[2]);
    psGeomMetadata.geometryDims = numOfPartitionsPerAxisValue;
    psGeomMetadata.geometryOrigin = llCoord;
    Result<FloatVec3> result = GeometryUtilities::CalculatePartitionLengthsOfBoundingBox({llCoord, urCoord}, numOfPartitionsPerAxisValue);
    if(result.valid())
    {
      psGeomMetadata.geometrySpacing = result.value();
    }
    psGeomMetadata.geometryUnits = geometry.getUnits();
    break;
  }
  case PartitioningMode::ExistingPartitionGrid: {
    auto pExistingPathValue = filterArgs.value<DataPath>(k_ExistingPartitionGridPath_Key);
    const auto& psGeom = dataStructure.getDataRefAs<ImageGeom>(pExistingPathValue);
    psGeomMetadata.geometryDims = psGeom.getDimensions();
    psGeomMetadata.geometryOrigin = psGeom.getOrigin();
    psGeomMetadata.geometrySpacing = psGeom.getSpacing();
    psGeomMetadata.geometryUnits = psGeom.getUnits();
    break;
  }
  }
  return {psGeomMetadata};
}

template <typename Geom>
Result<PSGeomInfo> GeneratePartitioningSchemeInfo(const Geom& geometry, const DataStructure& dataStructure, const Arguments& filterArgs)
{
  using namespace Parameters;

  auto pPartitioningModeValue = filterArgs.value<ChoicesParameter::ValueType>(k_PartitioningMode_Key);
  auto pNumberOfCellsPerAxisValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_NumberOfCellsPerAxis_Key);

  const SizeVec3 numOfPartitionsPerAxisValue = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]),
                                                static_cast<usize>(pNumberOfCellsPerAxisValue[2])};

  PSGeomInfo psGeomMetadata;

  switch(static_cast<PartitioningMode>(pPartitioningModeValue))
  {
  case PartitioningMode::Basic: {
    Result<FloatVec3> originResult;
    if constexpr(std::is_same_v<Geom, ImageGeom> || std::is_same_v<Geom, RectGridGeom>)
    {
      originResult = {geometry.getOrigin()};
    }
    else
    {
      originResult = GeometryUtilities::CalculateNodeBasedPartitionSchemeOrigin(geometry);
    }

    Result<FloatVec3> pLengthResult = GeometryUtilities::CalculatePartitionLengthsByPartitionCount(geometry, numOfPartitionsPerAxisValue);
    if(originResult.valid() && pLengthResult.valid())
    {
      psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]), static_cast<usize>(pNumberOfCellsPerAxisValue[2])};
      psGeomMetadata.geometryOrigin = originResult.value();
      psGeomMetadata.geometrySpacing = pLengthResult.value();
      psGeomMetadata.geometryUnits = geometry.getUnits();
    }
    break;
  }
  case PartitioningMode::Advanced: {
    auto pPartitioningSchemeOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_PartitionGridOrigin_Key);
    auto pCellLengthValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellLength_Key);

    psGeomMetadata.geometryDims = {static_cast<usize>(pNumberOfCellsPerAxisValue[0]), static_cast<usize>(pNumberOfCellsPerAxisValue[1]), static_cast<usize>(pNumberOfCellsPerAxisValue[2])};
    psGeomMetadata.geometryOrigin = pPartitioningSchemeOriginValue;
    psGeomMetadata.geometrySpacing = pCellLengthValue;
    psGeomMetadata.geometryUnits = geometry.getUnits();
    break;
  }
  case PartitioningMode::BoundingBox: {
    auto pMinGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MinGridCoord_Key);
    auto pMaxGridCoordValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MaxGridCoord_Key);

    psGeomMetadata.geometryDims = numOfPartitionsPerAxisValue;
    psGeomMetadata.geometryOrigin = pMinGridCoordValue;
    const FloatVec3 llCoord(pMinGridCoordValue[0], pMinGridCoordValue[1], pMinGridCoordValue[2]);
    const FloatVec3 urCoord(pMaxGridCoordValue[0], pMaxGridCoordValue[1], pMaxGridCoordValue[2]);

    Result<FloatVec3> result = GeometryUtilities::CalculatePartitionLengthsOfBoundingBox({llCoord, urCoord}, numOfPartitionsPerAxisValue);
    if(result.valid())
    {
      psGeomMetadata.geometrySpacing = result.value();
    }

    psGeomMetadata.geometryUnits = geometry.getUnits();
    break;
  }
  case PartitioningMode::ExistingPartitionGrid: {
    auto pExistingPartitioningSchemePathValue = filterArgs.value<DataPath>(k_ExistingPartitionGridPath_Key);
    const auto& psGeom = dataStructure.getDataRefAs<ImageGeom>(pExistingPartitioningSchemePathValue);
    psGeomMetadata.geometryDims = psGeom.getDimensions();
    psGeomMetadata.geometryOrigin = psGeom.getOrigin();
    psGeomMetadata.geometrySpacing = psGeom.getSpacing();
    psGeomMetadata.geometryUnits = psGeom.getUnits();
    break;
  }
  }

  return {psGeomMetadata};
}

} // namespace nx::core::PartitionUtilities
