#include "ComputeCoordinateThresholdScanline.hpp"

#include "ComputeCoordinateThreshold.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <algorithm>
#include <functional>
#include <memory>

using namespace nx::core;

namespace
{
// The fixed staging buffer bounds scratch memory for ImageGeom mask creation.
constexpr usize k_ChunkTuples = 65536;

/**
 * @brief Writes an ImageGeom mask in bounded batches.
 * @param imageGeom Supplies dimensions, origin, and spacing.
 * @param mask Receives one value per cell.
 * @param shouldInvert True to reverse mask values.
 * @param isInBounds Tests each cell corner.
 * @param shouldCancel Signals cancellation between batches.
 * @return Success, or an output bulk-I/O error.
 *
 * A cell passes only when all eight corners pass. Cancellation returns success
 * after completed batches. Later cells are not written.
 */
Result<> ComputeImageMask(const ImageGeom& imageGeom, UInt8AbstractDataStore& mask, bool shouldInvert, const std::function<uint8(float32, float32, float32)>& isInBounds,
                          const std::atomic_bool& shouldCancel)
{
  const uint8 trueValue = shouldInvert ? 0 : 1;
  const uint8 falseValue = shouldInvert ? 1 : 0;
  const usize xCells = imageGeom.getNumXCells();
  const usize yCells = imageGeom.getNumYCells();
  const usize xyCells = xCells * yCells;
  const usize totalCells = imageGeom.getNumberOfCells();
  const FloatVec3 spacing = imageGeom.getSpacing();
  const FloatVec3 origin = imageGeom.getOrigin();
  auto maskBuffer = std::make_unique<uint8[]>(k_ChunkTuples);
  for(usize offset = 0; offset < totalCells; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize chunkCount = std::min(k_ChunkTuples, totalCells - offset);
    for(usize chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++)
    {
      const usize tupleIndex = offset + chunkIndex;
      const usize zIndex = tupleIndex / xyCells;
      const usize xyIndex = tupleIndex % xyCells;
      const usize yIndex = xyIndex / xCells;
      const usize xIndex = xyIndex % xCells;

      // The arithmetic and corner order match the direct ImageGeom implementation.
      const float32 minXValue = xIndex * spacing[0] + origin[0];
      const float32 minYValue = yIndex * spacing[1] + origin[1];
      const float32 minZValue = zIndex * spacing[2] + origin[2];
      const float32 maxXValue = xIndex * spacing[0] + origin[0] + spacing[0];
      const float32 maxYValue = yIndex * spacing[1] + origin[1] + spacing[1];
      const float32 maxZValue = zIndex * spacing[2] + origin[2] + spacing[2];

      uint8 inBoundsVertexCount = 0;
      inBoundsVertexCount += isInBounds(minXValue, minYValue, minZValue);
      inBoundsVertexCount += isInBounds(maxXValue, minYValue, minZValue);
      inBoundsVertexCount += isInBounds(minXValue, maxYValue, minZValue);
      inBoundsVertexCount += isInBounds(minXValue, minYValue, maxZValue);
      inBoundsVertexCount += isInBounds(maxXValue, maxYValue, maxZValue);
      inBoundsVertexCount += isInBounds(minXValue, maxYValue, maxZValue);
      inBoundsVertexCount += isInBounds(maxXValue, minYValue, maxZValue);
      inBoundsVertexCount += isInBounds(maxXValue, maxYValue, minZValue);
      maskBuffer[chunkIndex] = inBoundsVertexCount == 8 ? trueValue : falseValue;
    }

    Result<> writeResult = mask.copyFromBuffer(offset, nonstd::span<const uint8>(maskBuffer.get(), chunkCount));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  return {};
}
} // namespace

ComputeCoordinateThresholdScanline::ComputeCoordinateThresholdScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       const ComputeCoordinateThresholdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeCoordinateThresholdScanline::~ComputeCoordinateThresholdScanline() noexcept = default;

Result<> ComputeCoordinateThresholdScanline::operator()()
{
  std::function<uint8(float32, float32, float32)> isInBounds;
  switch(static_cast<ComputeCoordinateThreshold::BoundsType>(m_InputValues->ShapeType))
  {
  case ComputeCoordinateThreshold::BoundsType::Rectangle: {
    const VectorFloat32Parameter::ValueType minPoint = m_InputValues->MinCoord;
    const VectorFloat32Parameter::ValueType maxPoint = m_InputValues->MaxCoord;
    isInBounds = [minPoint, maxPoint](float32 x, float32 y, float32 z) -> uint8 {
      if(minPoint[0] > x || maxPoint[0] < x)
      {
        return 0;
      }
      if(minPoint[1] > y || maxPoint[1] < y)
      {
        return 0;
      }
      if(minPoint[2] > z || maxPoint[2] < z)
      {
        return 0;
      }
      return 1;
    };
    break;
  }
  case ComputeCoordinateThreshold::BoundsType::Sphere: {
    const VectorFloat32Parameter::ValueType sphereInfo = m_InputValues->SphereInfo;
    isInBounds = [sphereInfo](float32 x, float32 y, float32 z) -> uint8 {
      const float32 xDiff = x - sphereInfo[0];
      const float32 yDiff = y - sphereInfo[1];
      const float32 zDiff = z - sphereInfo[2];
      const float32 totalDiff = (xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff);
      return totalDiff > (sphereInfo[3] * sphereInfo[3]) ? 0 : 1;
    };
  }
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);
  auto& mask = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->MaskArrayPath).getDataStoreRef();
  return ComputeImageMask(imageGeom, mask, m_InputValues->Invert, isInBounds, m_ShouldCancel);
}
