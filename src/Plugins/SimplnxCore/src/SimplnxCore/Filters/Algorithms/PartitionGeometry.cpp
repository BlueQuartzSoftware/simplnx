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
constexpr int32 k_PartitionOriginNotFinite = -86171;
constexpr int32 k_PartitionCoordinateNotFinite = -86172;
constexpr int32 k_PartitionIdRange = -86173;
constexpr int32 k_PartitionSpacingNotFinite = -86211;
constexpr int32 k_PartitionSpacingNotPositive = -86212;
constexpr int32 k_PartitionGridProductTooLarge = -86213;
constexpr int32 k_PartitionCountTooLarge = -86214;
constexpr int32 k_PartitionIndexConversion = -86215;

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
      if(!std::isfinite(origin[axis]))
      {
        return MakeErrorResult(k_PartitionOriginNotFinite, fmt::format("Partition grid '{}' {} origin must be finite. The value is {}.", path.toString(), "XYZ"[axis], origin[axis]));
      }
      if(!std::isfinite(spacing[axis]))
      {
        return MakeErrorResult(k_PartitionSpacingNotFinite, fmt::format("Partition grid '{}' {} spacing must be finite. The value is {}.", path.toString(), "XYZ"[axis], spacing[axis]));
      }
      if(spacing[axis] <= 0)
      {
        return MakeErrorResult(k_PartitionSpacingNotPositive, fmt::format("Partition grid '{}' {} spacing must be greater than 0. The value is {}.", path.toString(), "XYZ"[axis], spacing[axis]));
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
          return MakeErrorResult(k_PartitionGridProductTooLarge, fmt::format("The total number of cells in partition grid '{}' is too large. The dimensions are {}. Use a smaller grid.",
                                                                             path.toString(), StringUtilities::formatDimensions3D(dimensions)));
        }
        product *= dimensions[axis];
      }
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
Result<usize> ComputeAxisIndex(T coordinate, float32 origin, float32 spacing, usize axis, const DataPath& inputPath, usize elementIndex)
{
  const T coordinateIndex = std::floor((coordinate - origin) / spacing);
  const float64 exclusiveBound = std::ldexp(1.0, std::numeric_limits<usize>::digits);
  if(!std::isfinite(coordinateIndex) || coordinateIndex < 0 || static_cast<float64>(coordinateIndex) >= exclusiveBound)
  {
    return MakeErrorResult<usize>(
        k_PartitionIndexConversion,
        fmt::format("Partition Geometry cannot map the {} coordinate at element index {} of input geometry '{}' to a partition. The value is {}. Check the partition grid origin and spacing.",
                    "XYZ"[axis], elementIndex, inputPath.toString(), coordinate));
  }
  return {static_cast<usize>(coordinateIndex)};
}

template <class T>
Result<std::optional<usize>> ComputePartitionIndex(const ImageGeom& grid, const Point3D<T>& coordinate, const DataPath& inputPath, usize elementIndex)
{
  const auto dimensions = grid.getDimensions();
  const auto origin = grid.getOrigin();
  const auto spacing = grid.getSpacing();
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(!std::isfinite(coordinate[axis]))
    {
      return MakeErrorResult<std::optional<usize>>(
          k_PartitionCoordinateNotFinite,
          fmt::format("Input geometry '{}' has an invalid {} coordinate at element index {}. The value is {}. It must be finite.", inputPath.toString(), "XYZ"[axis], elementIndex, coordinate[axis]));
    }
  }
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
    auto index = ComputeAxisIndex(coordinate[axis], origin[axis], spacing[axis], axis, inputPath, elementIndex);
    if(index.invalid())
    {
      return ConvertInvalidResult<std::optional<usize>>(std::move(index));
    }
    indices[axis] = index.value();
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

Result<> nx::core::ValidateRectGridPartitionCoordinates(const RectGridGeom& inputGrid, const ImageGeom& partitionGrid, const DataPath& inputPath, const std::atomic_bool& shouldCancel)
{
  const auto dimensions = partitionGrid.getDimensions();
  const auto origin = partitionGrid.getOrigin();
  const auto spacing = partitionGrid.getSpacing();
  const std::array bounds = {&inputGrid.getXBoundsRef(), &inputGrid.getYBoundsRef(), &inputGrid.getZBoundsRef()};
  std::array<float32, 4096> buffer;
  for(usize axis = 0; axis < bounds.size(); ++axis)
  {
    const auto& store = bounds[axis]->getDataStoreRef();
    float64 previous = 0;
    for(usize offset = 0; offset < store.getSize();)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(buffer.size(), store.getSize() - offset);
      auto read = store.copyIntoBuffer(offset, nonstd::span<float32>(buffer.data(), count));
      if(read.invalid())
      {
        return read;
      }
      for(usize position = 0; position < count; ++position)
      {
        const usize index = offset + position;
        const float64 value = buffer[position];
        if(!std::isfinite(value))
        {
          return MakeErrorResult(k_PartitionCoordinateNotFinite,
                                 fmt::format("Input geometry '{}' has an invalid {} bound at element index {}. The value is {}. It must be finite.", inputPath.toString(), "XYZ"[axis], index, value));
        }
        if(index > 0)
        {
          // A rectilinear cell center uses only the adjacent bounds on its axis.
          const float64 center = 0.5 * (previous + value);
          if(!IsOutsideAxis(center, dimensions[axis], origin[axis], spacing[axis]))
          {
            auto checked = ComputeAxisIndex(center, origin[axis], spacing[axis], axis, inputPath, index - 1);
            if(checked.invalid())
            {
              return ConvertResult(std::move(checked));
            }
          }
        }
        previous = value;
      }
      offset += count;
    }
  }
  return {};
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
