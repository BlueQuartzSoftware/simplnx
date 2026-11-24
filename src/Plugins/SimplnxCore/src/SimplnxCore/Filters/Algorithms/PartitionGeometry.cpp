#include "PartitionGeometry.hpp"

#include "PartitionGeometryDirect.hpp"
#include "PartitionGeometryScanline.hpp"

#include "SimplnxCore/Filters/PartitionGeometryFilter.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

using namespace nx::core;

namespace
{
constexpr int32 k_PartitionIdRange = -86173;
constexpr int32 k_PartitionSpacingNotPositive = -86212;
constexpr int32 k_PartitionGridProductTooLarge = -86213;
constexpr int32 k_PartitionCountTooLarge = -86214;
constexpr int32 k_PartitionIndexConversion = -86215;

Result<> ValidatePreparedPartitionGrid(const ImageGeom& grid, const DataPath& path, const PartitionGeometryInputValues& input, bool existing)
{
  const auto dimensions = grid.getDimensions();
  const auto spacing = grid.getSpacing();
  if(input.PartitioningMode != static_cast<ChoicesParameter::ValueType>(PartitionGeometryFilter::PartitioningMode::Advanced))
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      if(std::isfinite(spacing[axis]) && spacing[axis] <= 0)
      {
        return MakeErrorResult(k_PartitionSpacingNotPositive, fmt::format("Partition grid '{}' {} spacing must be greater than 0. The value is {}.", path.toString(), "XYZ"[axis], spacing[axis]));
      }
    }
  }
  if(existing)
  {
    usize product = 1;
    for(usize axis = 0; axis < 3; ++axis)
    {
      if(dimensions[axis] > std::numeric_limits<usize>::max() / product)
      {
        return MakeErrorResult(k_PartitionGridProductTooLarge, fmt::format("The total number of cells in partition grid '{}' is too large. The dimensions are {}. Use a smaller grid.", path.toString(),
                                                                           StringUtilities::formatDimensions3D(dimensions)));
      }
      product *= dimensions[axis];
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

// Keep the same coordinate arithmetic as ImageGeom::getIndex().
template <class T>
bool IsOutsideAxis(T coordinate, usize dimension, float32 origin, float32 spacing)
{
  return coordinate < origin || coordinate > (static_cast<T>(dimension) * spacing + origin);
}

template <class T>
std::optional<usize> TryConvertPartitionAxis(T coordinate, float32 origin, float32 spacing)
{
  const T coordinateIndex = std::floor((coordinate - origin) / spacing);
  const float64 exclusiveBound = std::ldexp(1.0, std::numeric_limits<usize>::digits);
  if(!std::isfinite(coordinateIndex) || coordinateIndex < 0 || static_cast<float64>(coordinateIndex) >= exclusiveBound)
  {
    return {};
  }
  return static_cast<usize>(coordinateIndex);
}

template <class T>
Result<std::optional<usize>> ComputePartitionIndex(const ImageGeom& grid, const Point3D<T>& coordinate, const DataPath& inputPath, usize elementIndex)
{
  const auto dimensions = grid.getDimensions();
  const auto origin = grid.getOrigin();
  const auto spacing = grid.getSpacing();
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(IsOutsideAxis(coordinate[axis], dimensions[axis], origin[axis], spacing[axis]))
    {
      return {std::optional<usize>{}};
    }
  }
  std::array<usize, 3> indices = {};
  for(usize axis = 0; axis < 3; ++axis)
  {
    const auto index = TryConvertPartitionAxis(coordinate[axis], origin[axis], spacing[axis]);
    if(!index.has_value())
    {
      return MakeErrorResult<std::optional<usize>>(k_PartitionIndexConversion, fmt::format("Partition Geometry cannot map the {} coordinate at element index {} of input geometry '{}' to partition "
                                                                                           "grid '{}'. The value is {}. Check the coordinate and the partition grid origin and spacing.",
                                                                                           "XYZ"[axis], elementIndex, inputPath.toString(), grid.getName(), coordinate[axis]));
    }
    indices[axis] = *index;
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
  const auto maximumId = std::numeric_limits<int32>::max();
  if(maximumIndex > static_cast<usize>(maximumId))
  {
    return MakeErrorResult(k_PartitionCountTooLarge, fmt::format("The partition count {} is too large. Use at most {} partitions.", partitionCount, static_cast<usize>(maximumId) + 1));
  }
  if(static_cast<int64>(startingFeatureId) + static_cast<int64>(maximumIndex) > maximumId)
  {
    return MakeErrorResult(k_PartitionIdRange, fmt::format("Starting Feature ID {} and partition count {} give a Feature ID above {}. Decrease Starting Feature ID or use fewer partitions.",
                                                           startingFeatureId, partitionCount, maximumId));
  }
  return {};
}

Result<std::optional<usize>> nx::core::ComputeCheckedPartitionIndex(const ImageGeom& grid, const Point3D<float64>& coordinate, const DataPath& inputPath, usize elementIndex)
{
  return ComputePartitionIndex(grid, coordinate, inputPath, elementIndex);
}

Result<std::optional<usize>> nx::core::ComputeCheckedPartitionIndex(const ImageGeom& grid, const Point3Df& coordinate, const DataPath& inputPath, usize elementIndex)
{
  return ComputePartitionIndex(grid, coordinate, inputPath, elementIndex);
}

std::optional<usize> nx::core::detail::TryMapPartitionAxis(float64 coordinate, usize dimension, float32 origin, float32 spacing)
{
  if(IsOutsideAxis(coordinate, dimension, origin, spacing))
  {
    return dimension;
  }
  const auto index = TryConvertPartitionAxis(coordinate, origin, spacing);
  if(index.has_value())
  {
    return std::min(*index, dimension);
  }
  return {};
}

detail::RectGridPartitionCenters::RectGridPartitionCenters(const RectGridGeom& grid)
: m_Bounds{&grid.getXBoundsRef().getDataStoreRef(), &grid.getYBoundsRef().getDataStoreRef(), &grid.getZBoundsRef().getDataStoreRef()}
, m_Dimensions(grid.getDimensions())
{
}

Result<float64> detail::RectGridPartitionCenters::readAxis(usize axis, usize index)
{
  auto& cache = m_Caches[axis];
  if(cache.Size == 0 || index < cache.Start || index - cache.Start >= cache.Size - 1)
  {
    // Include the upper bound of the last cell in each block.
    constexpr usize k_CellsPerBlock = k_ValuesPerBlock - 1;
    const usize start = (index / k_CellsPerBlock) * k_CellsPerBlock;
    const auto& store = *m_Bounds[axis];
    const usize available = start < store.getSize() ? store.getSize() - start : 0;
    // Request both bounds even when the store is too short so it returns its range error.
    const usize count = std::clamp(available, index - start + 2, k_ValuesPerBlock);
    auto read = store.copyIntoBuffer(start, nonstd::span<float32>(cache.Values.data(), count));
    if(read.invalid())
    {
      return ConvertInvalidResult<float64>(std::move(read));
    }
    cache.Start = start;
    cache.Size = count;
  }
  const usize localIndex = index - cache.Start;
  return {0.5 * (static_cast<float64>(cache.Values[localIndex]) + cache.Values[localIndex + 1])};
}

Result<Point3D<float64>> detail::RectGridPartitionCenters::readTuple(usize index)
{
  const std::array cell = {index % m_Dimensions[0], (index / m_Dimensions[0]) % m_Dimensions[1], index / (m_Dimensions[0] * m_Dimensions[1])};
  Point3D<float64> coordinate;
  for(usize axis = 0; axis < cell.size(); ++axis)
  {
    auto component = readAxis(axis, cell[axis]);
    if(component.invalid())
    {
      return ConvertInvalidResult<Point3D<float64>>(std::move(component));
    }
    coordinate[axis] = component.value();
  }
  return {coordinate};
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
  const auto partitioningMode = static_cast<PartitionGeometryFilter::PartitioningMode>(m_InputValues->PartitioningMode);
  const bool existing = partitioningMode == PartitionGeometryFilter::PartitioningMode::ExistingPartitionGrid;
  const DataPath& gridPath = existing ? m_InputValues->ExistingPartitionGridPath : m_InputValues->PartitionGridGeomPath;
  const auto& grid = m_DataStructure.getDataRefAs<ImageGeom>(gridPath);
  const auto dimensions = grid.getDimensions();
  const DataPath partitionIdsPath = m_InputValues->InputGeomCellAMPath.createChildPath(m_InputValues->PartitionIdsArrayName);
  auto& partitionIds = m_DataStructure.getDataRefAs<Int32Array>(partitionIdsPath);
  if(existing && (dimensions[0] == 0 || dimensions[1] == 0 || dimensions[2] == 0))
  {
    // No input element can map to an empty grid. Fill the IDs without reading coordinates.
    auto& store = partitionIds.getDataStoreRef();
    std::array<int32, 4096> buffer;
    buffer.fill(m_InputValues->OutOfBoundsFeatureID);
    for(usize offset = 0; offset < store.getSize() && !m_ShouldCancel;)
    {
      const usize count = std::min(buffer.size(), store.getSize() - offset);
      auto result = store.copyFromBuffer(offset, nonstd::span<const int32>(buffer.data(), count));
      if(result.invalid())
      {
        return result;
      }
      offset += count;
    }
    return {};
  }

  auto gridResult = ValidatePreparedPartitionGrid(grid, gridPath, *m_InputValues, existing);
  if(gridResult.invalid())
  {
    return gridResult;
  }

  const IDataArray* partitionGridFeatureIdsArray = nullptr;
  if(!existing)
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
  return DispatchAlgorithm<PartitionGeometryDirect, PartitionGeometryScanline>({&partitionIds, partitionGridFeatureIdsArray, verticesArray, maskArray}, m_DataStructure, m_MessageHandler,
                                                                               m_ShouldCancel, m_InputValues);
}
