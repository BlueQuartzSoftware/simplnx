#include "ComputeCoordinateThresholdDirect.hpp"

#include "ComputeCoordinateThreshold.hpp"
#include "ComputeCoordinateThresholdScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
/**
 * @brief Tests whether both cell endpoints are inside a bound interval.
 * @param minValue Identifies the cell lower coordinate.
 * @param maxValue Identifies the cell upper coordinate.
 * @param minBound Identifies the bound lower coordinate.
 * @param maxBound Identifies the bound upper coordinate.
 * @return True if both endpoints are inside the bounds.
 */
bool AreEndpointsInBounds(float32 minValue, float32 maxValue, float32 minBound, float32 maxBound)
{
  return !(minBound > minValue || maxBound < minValue || minBound > maxValue || maxBound < maxValue);
}

/**
 * @brief Writes a rectangular ImageGeom mask to contiguous storage.
 * @param imageGeom Supplies dimensions, origin, and spacing.
 * @param mask Receives one value per cell.
 * @param shouldInvert True to reverse mask values.
 * @param minPoint Supplies minimum x, y, and z bounds.
 * @param maxPoint Supplies maximum x, y, and z bounds.
 * @param shouldCancel Signals cancellation between Z slices.
 * @return Success.
 * @pre mask addresses an in-memory buffer for every ImageGeom cell.
 *
 * Cancellation returns success after complete Z slices. Later slices are not written.
 */
Result<> ComputeRectangleMask(const ImageGeom& imageGeom, uint8* mask, bool shouldInvert, const VectorFloat32Parameter::ValueType& minPoint, const VectorFloat32Parameter::ValueType& maxPoint,
                              const std::atomic_bool& shouldCancel)
{
  const uint8 trueValue = shouldInvert ? 0 : 1;
  const uint8 falseValue = shouldInvert ? 1 : 0;
  const usize xCells = imageGeom.getNumXCells();
  const usize yCells = imageGeom.getNumYCells();
  const usize zCells = imageGeom.getNumZCells();
  const usize xyCells = xCells * yCells;
  const FloatVec3 spacing = imageGeom.getSpacing();
  const FloatVec3 origin = imageGeom.getOrigin();

  for(usize zIndex = 0; zIndex < zCells; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const float32 minZValue = zIndex * spacing[2] + origin[2];
    const float32 maxZValue = zIndex * spacing[2] + origin[2] + spacing[2];
    const bool zInBounds = AreEndpointsInBounds(minZValue, maxZValue, minPoint[2], maxPoint[2]);
    const usize zOffset = zIndex * xyCells;
    for(usize yIndex = 0; yIndex < yCells; yIndex++)
    {
      uint8* rowMask = mask + zOffset + (yIndex * xCells);
      const float32 minYValue = yIndex * spacing[1] + origin[1];
      const float32 maxYValue = yIndex * spacing[1] + origin[1] + spacing[1];
      if(!zInBounds || !AreEndpointsInBounds(minYValue, maxYValue, minPoint[1], maxPoint[1]))
      {
        std::fill_n(rowMask, xCells, falseValue);
        continue;
      }

      for(usize xIndex = 0; xIndex < xCells; xIndex++)
      {
        const float32 minXValue = xIndex * spacing[0] + origin[0];
        const float32 maxXValue = xIndex * spacing[0] + origin[0] + spacing[0];
        rowMask[xIndex] = AreEndpointsInBounds(minXValue, maxXValue, minPoint[0], maxPoint[0]) ? trueValue : falseValue;
      }
    }
  }

  return {};
}

/**
 * @struct SpherePredicate
 * @brief Tests whether a point is inside a selected sphere.
 */
struct SpherePredicate
{
  // This value stores center x, y, z and radius.
  VectorFloat32Parameter::ValueType sphereInfo;

  /**
   * @brief Tests one point against the sphere.
   * @param x Identifies the x coordinate.
   * @param y Identifies the y coordinate.
   * @param z Identifies the z coordinate.
   * @return One if the point is inside or on the sphere. Returns zero otherwise.
   */
  uint8 operator()(float32 x, float32 y, float32 z) const
  {
    const float32 xDiff = x - sphereInfo[0];
    const float32 yDiff = y - sphereInfo[1];
    const float32 zDiff = z - sphereInfo[2];
    const float32 totalDiff = (xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff);
    return totalDiff > (sphereInfo[3] * sphereInfo[3]) ? 0 : 1;
  }
};

/**
 * @brief Writes an ImageGeom mask from a corner predicate.
 * @tparam PredicateT Specifies the point-in-bounds predicate type.
 * @param imageGeom Supplies dimensions, origin, and spacing.
 * @param mask Receives one value per cell.
 * @param shouldInvert True to reverse mask values.
 * @param isInBounds Tests each cell corner.
 * @param shouldCancel Signals cancellation between Z slices.
 * @return Success.
 * @pre mask addresses an in-memory buffer for every ImageGeom cell.
 *
 * A cell passes only when all eight corners pass. Cancellation returns success
 * after complete Z slices. Later slices are not written.
 */
template <typename PredicateT>
Result<> ComputeCornerMask(const ImageGeom& imageGeom, uint8* mask, bool shouldInvert, const PredicateT& isInBounds, const std::atomic_bool& shouldCancel)
{
  const uint8 trueValue = shouldInvert ? 0 : 1;
  const uint8 falseValue = shouldInvert ? 1 : 0;
  const usize xCells = imageGeom.getNumXCells();
  const usize yCells = imageGeom.getNumYCells();
  const usize zCells = imageGeom.getNumZCells();
  const usize xyCells = xCells * yCells;
  const FloatVec3 spacing = imageGeom.getSpacing();
  const FloatVec3 origin = imageGeom.getOrigin();

  for(usize zIndex = 0; zIndex < zCells; zIndex++)
  {
    if(shouldCancel)
    {
      return {};
    }

    const float32 minZValue = zIndex * spacing[2] + origin[2];
    const float32 maxZValue = zIndex * spacing[2] + origin[2] + spacing[2];
    const usize zOffset = zIndex * xyCells;
    for(usize yIndex = 0; yIndex < yCells; yIndex++)
    {
      const float32 minYValue = yIndex * spacing[1] + origin[1];
      const float32 maxYValue = yIndex * spacing[1] + origin[1] + spacing[1];
      uint8* rowMask = mask + zOffset + (yIndex * xCells);
      for(usize xIndex = 0; xIndex < xCells; xIndex++)
      {
        const float32 minXValue = xIndex * spacing[0] + origin[0];
        const float32 maxXValue = xIndex * spacing[0] + origin[0] + spacing[0];

        uint8 inBoundsVertexCount = 0;
        inBoundsVertexCount += isInBounds(minXValue, minYValue, minZValue);
        inBoundsVertexCount += isInBounds(maxXValue, minYValue, minZValue);
        inBoundsVertexCount += isInBounds(minXValue, maxYValue, minZValue);
        inBoundsVertexCount += isInBounds(minXValue, minYValue, maxZValue);
        inBoundsVertexCount += isInBounds(maxXValue, maxYValue, maxZValue);
        inBoundsVertexCount += isInBounds(minXValue, maxYValue, maxZValue);
        inBoundsVertexCount += isInBounds(maxXValue, minYValue, maxZValue);
        inBoundsVertexCount += isInBounds(maxXValue, maxYValue, minZValue);
        rowMask[xIndex] = inBoundsVertexCount == 8 ? trueValue : falseValue;
      }
    }
  }

  return {};
}
} // namespace

ComputeCoordinateThresholdDirect::ComputeCoordinateThresholdDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                   const ComputeCoordinateThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeCoordinateThresholdDirect::~ComputeCoordinateThresholdDirect() noexcept = default;

Result<> ComputeCoordinateThresholdDirect::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);
  auto& maskStore = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->MaskArrayPath).getDataStoreRef();
  auto* inMemoryMask = dynamic_cast<UInt8DataStore*>(&maskStore);
  if(inMemoryMask == nullptr)
  {
    // A forced direct test can receive an out-of-core store. The scanline fallback keeps I/O bounded.
    return ComputeCoordinateThresholdScanline(m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues)();
  }

  switch(static_cast<ComputeCoordinateThreshold::BoundsType>(m_InputValues->ShapeType))
  {
  case ComputeCoordinateThreshold::BoundsType::Rectangle:
    return ComputeRectangleMask(imageGeom, inMemoryMask->data(), m_InputValues->Invert, m_InputValues->MinCoord, m_InputValues->MaxCoord, m_ShouldCancel);
  case ComputeCoordinateThreshold::BoundsType::Sphere:
    return ComputeCornerMask(imageGeom, inMemoryMask->data(), m_InputValues->Invert, SpherePredicate{m_InputValues->SphereInfo}, m_ShouldCancel);
  }

  return {};
}
