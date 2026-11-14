#include "PartitionGeometry.hpp"

#include "PartitionGeometryDirect.hpp"
#include "PartitionGeometryScanline.hpp"

#include "SimplnxCore/Filters/PartitionGeometryFilter.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <cmath>
#include <limits>

using namespace nx::core;

namespace
{
constexpr int32 k_PartitionGridRepresentation = -86171;
constexpr int32 k_PartitionCoordinateRepresentation = -86172;
constexpr int32 k_PartitionIdRange = -86173;

Result<> ValidatePreparedPartitionGrid(const DataStructure& dataStructure, const PartitionGeometryInputValues& input)
{
  const bool existing = input.PartitioningMode == static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid);
  const DataPath& path = existing ? input.ExistingPartitionGridPath : input.PartitionGridGeomPath;
  const auto& grid = dataStructure.getDataRefAs<ImageGeom>(path);
  const auto dimensions = grid.getDimensions();
  const auto spacing = grid.getSpacing();
  const auto origin = grid.getOrigin();
  if(input.PartitioningMode != static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::Advanced))
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      if(!std::isfinite(origin[axis]) || !std::isfinite(spacing[axis]) || spacing[axis] <= 0)
      {
        return MakeErrorResult(k_PartitionGridRepresentation, fmt::format("Partition grid '{}' {} requires finite origin and positive finite spacing. Origin={}, spacing={}.", path.toString(),
                                                                          "XYZ"[axis], origin[axis], spacing[axis]));
      }
    }
  }
  if(existing)
  {
    usize product = 1;
    if(dimensions[0] == 0 || dimensions[1] == 0 || dimensions[2] == 0)
    {
      product = 0;
    }
    else
    {
      for(usize axis = 0; axis < 3; ++axis)
      {
        if(dimensions[axis] > std::numeric_limits<usize>::max() / product)
        {
          return MakeErrorResult(k_PartitionGridRepresentation, fmt::format("Partition grid '{}' tuple product overflows: {}.", path.toString(), StringUtilities::formatDimensions3D(dimensions)));
        }
        product *= dimensions[axis];
      }
    }
    if(product > std::numeric_limits<usize>::max() / sizeof(int32))
    {
      return MakeErrorResult(k_PartitionGridRepresentation, fmt::format("Partition grid '{}' byte count overflows: partition count={}, element bytes={}.", path.toString(), product, sizeof(int32)));
    }
    auto result = detail::CheckPartitionIdRange(product, input.StartingFeatureID);
    if(result.invalid())
    {
      result.errors().front().message = fmt::format("Partition grid '{}': {}", path.toString(), result.errors().front().message);
    }
    return result;
  }
  return {};
}

// Keep subtraction, division and bounds arithmetic in the caller's coordinate type.
template <typename T>
Result<std::optional<usize>> ComputePartitionIndex(const ImageGeom& grid, const Point3D<T>& coordinate)
{
  const auto dimensions = grid.getDimensions();
  const auto origin = grid.getOrigin();
  const auto spacing = grid.getSpacing();
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(coordinate[axis] < origin[axis] || coordinate[axis] > (static_cast<T>(dimensions[axis]) * spacing[axis] + origin[axis]))
    {
      return {std::optional<usize>{}};
    }
  }
  std::array<usize, 3> indices = {};
  for(usize axis = 0; axis < 3; ++axis)
  {
    const T quotient = std::floor((coordinate[axis] - origin[axis]) / spacing[axis]);
    const float64 exclusiveBound = std::ldexp(1.0, std::numeric_limits<usize>::digits);
    if(!std::isfinite(quotient) || quotient < 0 || static_cast<float64>(quotient) >= exclusiveBound)
    {
      const auto paths = grid.getDataPaths();
      const std::string path = paths.empty() ? grid.getName() : paths.front().toString();
      return MakeErrorResult<std::optional<usize>>(
          k_PartitionCoordinateRepresentation,
          fmt::format("Partition grid '{}' {} coordinate {} gives quotient {}. The quotient must be finite, nonnegative and below exclusive bound {} (origin={}, spacing={}).", path, "XYZ"[axis],
                      coordinate[axis], quotient, exclusiveBound, origin[axis], spacing[axis]));
    }
    indices[axis] = static_cast<usize>(quotient);
    if(indices[axis] >= dimensions[axis])
    {
      return {std::optional<usize>{}};
    }
  }
  return {std::optional<usize>{dimensions[1] * dimensions[0] * indices[2] + dimensions[0] * indices[1] + indices[0]}};
}
} // namespace

Result<> nx::core::detail::CheckPartitionIdRange(usize partitionCount, int32 startingFeatureId)
{
  if(partitionCount == 0)
  {
    return {};
  }
  const usize maximumIndex = partitionCount - 1;
  if(maximumIndex > static_cast<usize>(std::numeric_limits<int32>::max()) ||
     static_cast<int64>(startingFeatureId) > static_cast<int64>(std::numeric_limits<int32>::max()) - static_cast<int64>(maximumIndex))
  {
    return MakeErrorResult(k_PartitionIdRange, fmt::format("Starting Feature ID {} and partition count {} exceed the consumed int32 index or identifier limit {}.", startingFeatureId, partitionCount,
                                                           std::numeric_limits<int32>::max()));
  }
  return {};
}

Result<std::optional<usize>> nx::core::ComputeCheckedPartitionIndex(const ImageGeom& grid, const Point3D<float64>& coordinate)
{
  return ComputePartitionIndex(grid, coordinate);
}

Result<std::optional<usize>> nx::core::ComputeCheckedPartitionIndex(const ImageGeom& grid, const Point3Df& coordinate)
{
  return ComputePartitionIndex(grid, coordinate);
}

PartitionGeometry::PartitionGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

PartitionGeometry::~PartitionGeometry() noexcept = default;

Result<> PartitionGeometry::operator()()
{
  auto gridResult = ValidatePreparedPartitionGrid(m_DataStructure, *m_InputValues);
  if(gridResult.invalid())
  {
    return gridResult;
  }

  const DataPath partitionIdsPath = m_InputValues->InputGeomCellAMPath.createChildPath(m_InputValues->PartitionIdsArrayName);
  const auto* partitionIdsArray = m_DataStructure.getDataAs<Int32Array>(partitionIdsPath);

  const IDataArray* partitionGridFeatureIdsArray = nullptr;
  const auto partitioningMode = static_cast<PartitionGeometryFilter::PartitioningMode>(m_InputValues->PartitioningMode);
  if(partitioningMode != PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid)
  {
    const DataPath partitionGridFeatureIdsPath =
        m_InputValues->PartitionGridGeomPath.createChildPath(m_InputValues->PartitionGridCellAMName).createChildPath(m_InputValues->PartitionGridFeatureIDsArrayName);
    partitionGridFeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(partitionGridFeatureIdsPath);
  }

  const IDataArray* verticesArray = nullptr;
  const IGeometry& geometry = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->InputGeometryToPartition);
  switch(geometry.getGeomType())
  {
  case IGeometry::Type::Vertex:
  case IGeometry::Type::Edge:
  case IGeometry::Type::Triangle:
  case IGeometry::Type::Quad:
  case IGeometry::Type::Tetrahedral:
  case IGeometry::Type::Hexahedral: {
    const auto& nodeGeom = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->InputGeometryToPartition);
    verticesArray = nodeGeom.getVertices();
    break;
  }
  default:
    break;
  }

  const IDataArray* maskArray = m_InputValues->UseVertexMask ? m_DataStructure.getDataAs<BoolArray>(m_InputValues->VertexMaskPath) : nullptr;
  return DispatchAlgorithm<PartitionGeometryDirect, PartitionGeometryScanline>({partitionIdsArray, partitionGridFeatureIdsArray, verticesArray, maskArray}, m_DataStructure, m_MessageHandler,
                                                                               m_ShouldCancel, m_InputValues);
}
