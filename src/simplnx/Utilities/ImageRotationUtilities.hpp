#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/FilterMessenger.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/simplnx_export.hpp"

#include <Eigen/Dense>

#include <concepts>
#include <iostream>

namespace nx::core::ImageRotationUtilities
{
const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();

using Matrix3fR = Eigen::Matrix<float, 3, 3, Eigen::RowMajor>;
using Matrix4fR = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

using Vector3i64 = Eigen::Array<int64_t, 1, 3>;

struct RotateArgs
{
  USizeVec3 OriginalDims;
  FloatVec3 OriginalSpacing;
  FloatVec3 OriginalOrigin;
  int64_t xp = 0;
  int64_t yp = 0;
  int64_t zp = 0;
  float xRes = 0.0f;
  float yRes = 0.0f;
  float zRes = 0.0f;

  USizeVec3 TransformedDims;
  FloatVec3 TransformedSpacing;
  FloatVec3 TransformedOrigin;

  USizeVec3 outputDims;
  FloatVec3 outputSpacing;

  float outputXMin = 0.0f;
  float outputYMin = 0.0f;
  float outputZMin = 0.0f;
};

/**
 * @brief
 * @param transform
 * @return
 */
SIMPLNX_EXPORT std::string GenerateTransformationMatrixDescription(const ImageRotationUtilities::Matrix4fR& transform);

/**
 * @brief
 * @param precomputed
 * @return
 */
SIMPLNX_EXPORT Matrix4fR CopyPrecomputedToTransformationMatrix(const AbstractDataStore<float32>& precomputed);

/**
 * @brief
 * @param tableData
 * @return
 */
SIMPLNX_EXPORT Matrix4fR GenerateManualTransformationMatrix(const DynamicTableParameter::ValueType& tableData);

/**
 * @brief
 * @param pRotationValue
 * @return
 */
SIMPLNX_EXPORT Matrix4fR GenerateRotationTransformationMatrix(const VectorFloat32Parameter::ValueType& pRotationValue);

/**
 * @brief
 * @param pTranslationValue
 * @return
 */
SIMPLNX_EXPORT Matrix4fR GenerateTranslationTransformationMatrix(const VectorFloat32Parameter::ValueType& pTranslationValue);

/**
 * @brief
 * @param pScaleValue
 * @return
 */
SIMPLNX_EXPORT Matrix4fR GenerateScaleTransformationMatrix(const VectorFloat32Parameter::ValueType& pScaleValue);

/**
 * @brief Function to determine the min and max coordinates of the transformed Image Geometry using the bounding box.
 * @param imageGeomBoundingBox
 * @param transformationMatrix
 * @return
 */
SIMPLNX_EXPORT FloatVec6 DetermineMinMaxCoords(const BoundingBox3Df& imageGeomBoundingBox, const Matrix4fR& transformationMatrix);

/**
 * @brief Function to determine the min and max coordinates (bounding box) of the transformed Image Geometry.
 * @param imageGeometry
 * @param transformationMatrix
 * @return
 */
SIMPLNX_EXPORT FloatVec6 DetermineMinMaxCoords(const ImageGeom& imageGeometry, const Matrix4fR& transformationMatrix);

/**
 * @brief Finds the Cosine of the angle between 2 vectors
 * @tparam T
 * @param vectorA
 * @param vectorB
 * @return
 */
template <std::floating_point T>
T CosBetweenVectors(const Eigen::Vector3<T>& vectorA, const Eigen::Vector3<T>& vectorB)
{
  const T normA = vectorA.norm();
  const T normB = vectorB.norm();

  if(normA == static_cast<T>(0.0) || normB == static_cast<T>(0.0))
  {
    return static_cast<T>(1.0);
  }

  return vectorA.dot(vectorB) / (normA * normB);
}

/**
 * @brief Function for determining new ImageGeom Spacing between points for scaling
 * @param spacing
 * @param axisNew
 * @return spacing for a given axis.
 */
SIMPLNX_EXPORT float DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew);

/**
 * @brief Determines parameters for image rotation
 * @param imageGeom
 * @param transformationMatrix
 * @return New RotateArgs object
 */
SIMPLNX_EXPORT ImageRotationUtilities::RotateArgs CreateRotationArgs(const ImageGeom& imageGeom, const Matrix4fR& transformationMatrix);

/**
 * @brief
 * @tparam T
 * @param params
 * @param xyzIndex
 * @param sourceArray
 * @param compIndex
 * @return
 */
template <typename T>
T inline GetSourceArrayValue(const RotateArgs& params, Vector3i64 xyzIndex, const DataArray<T>& sourceArray, size_t compIndex)
{
  if(xyzIndex[0] < 0)
  {
    xyzIndex[0] = 0;
  }
  if(xyzIndex[0] >= params.xp)
  {
    xyzIndex[0] = params.xp - 1;
  }

  if(xyzIndex[1] < 0)
  {
    xyzIndex[1] = 0;
  }
  if(xyzIndex[1] >= params.yp)
  {
    xyzIndex[1] = params.yp - 1;
  }

  if(xyzIndex[2] < 0)
  {
    xyzIndex[2] = 0;
  }
  if(xyzIndex[2] >= params.zp)
  {
    xyzIndex[2] = params.zp - 1;
  }

  // Now just compute the proper index
  const usize index = (xyzIndex[2] * params.xp * params.yp) + (xyzIndex[1] * params.xp) + xyzIndex[0];
  return sourceArray[index * sourceArray.getNumberOfComponents() + compIndex];
}

/**
 * @brief
 * @param params
 * @param centerPoint
 * @param coord
 * @return
 */
SIMPLNX_EXPORT size_t FindOctant(const RotateArgs& params, const Point3Df& centerPoint, const Eigen::Array4f& coord);

using OctantOffsetArrayType = std::array<Vector3i64, 8>;

/* clang-format off */

static const OctantOffsetArrayType k_IndexOffset0 = {Vector3i64{-1, -1, -1}, Vector3i64{0, -1, -1}, Vector3i64{0, 0, -1}, Vector3i64{-1, 0, -1},
                                                     Vector3i64{-1, -1, 0},  Vector3i64{0, -1, 0},  Vector3i64{0, 0, 0},  Vector3i64{-1, 0, 0}};
static const OctantOffsetArrayType k_IndexOffset1 = {Vector3i64{0, -1, -1}, Vector3i64{1, -1, -1}, Vector3i64{1, 0, -1}, Vector3i64{0, 0, -1},
                                                     Vector3i64{0, -1, 0},  Vector3i64{1, -1, 0},  Vector3i64{1, 0, 0},  Vector3i64{0, 0, 0}};
static const OctantOffsetArrayType k_IndexOffset2 = {Vector3i64{0, 0, -1}, Vector3i64{1, 0, -1}, Vector3i64{1, 1, -1}, Vector3i64{0, 1, -1},
                                                     Vector3i64{0, 0, 0},  Vector3i64{1, 0, 0},  Vector3i64{1, 1, 0},  Vector3i64{0, 1, 0}};
static const OctantOffsetArrayType k_IndexOffset3 = {Vector3i64{-1, 0, -1}, Vector3i64{0, 0, -1}, Vector3i64{0, 1, -1}, Vector3i64{-1, 1, -1},
                                                     Vector3i64{-1, 0, 0},  Vector3i64{0, 0, 0},  Vector3i64{0, 1, 0},  Vector3i64{-1, 1, 0}};
static const OctantOffsetArrayType k_IndexOffset4 = {Vector3i64{-1, -1, 0}, Vector3i64{0, -1, 0}, Vector3i64{0, 0, 0}, Vector3i64{-1, 0, 0},
                                                     Vector3i64{-1, -1, 1}, Vector3i64{0, -1, 1}, Vector3i64{0, 0, 1}, Vector3i64{-1, 0, 1}};
static const OctantOffsetArrayType k_IndexOffset5 = {Vector3i64{0, -1, 0}, Vector3i64{1, -1, 0}, Vector3i64{1, 0, 0}, Vector3i64{0, 0, 0},
                                                     Vector3i64{0, -1, 1}, Vector3i64{1, -1, 1}, Vector3i64{1, 0, 1}, Vector3i64{0, 0, 1}};
static const OctantOffsetArrayType k_IndexOffset6 = {Vector3i64{0, 0, 0}, Vector3i64{1, 0, 0}, Vector3i64{1, 1, 0}, Vector3i64{0, 1, 0},
                                                     Vector3i64{0, 0, 1}, Vector3i64{1, 0, 1}, Vector3i64{1, 1, 1}, Vector3i64{0, 1, 1}};
static const OctantOffsetArrayType k_IndexOffset7 = {Vector3i64{-1, 0, 0}, Vector3i64{0, 0, 0}, Vector3i64{0, 1, 0}, Vector3i64{-1, -1, 0},
                                                     Vector3i64{-1, 0, 1}, Vector3i64{0, 0, 1}, Vector3i64{0, 1, 1}, Vector3i64{-1, -1, 1}};
static const std::array<OctantOffsetArrayType, 8> k_AllOctantOffsets{k_IndexOffset0, k_IndexOffset1, k_IndexOffset2, k_IndexOffset3, k_IndexOffset4, k_IndexOffset5, k_IndexOffset6, k_IndexOffset7};

/* clang-format on */

template <class T>
using AccumulationValueType = std::conditional_t<std::is_floating_point_v<T>, float64, int64>;

/**
 * @brief FindInterpolationValues
 * @tparam T
 * @param params
 * @param octant
 * @param oldIndicesU
 * @param oldCoords
 * @param sourceArray
 * @param pValues
 * @param uvw
 * @param hitVoxelCenterPoint
 */
template <typename T>
inline void FindInterpolationValues(const RotateArgs& params, size_t octant, SizeVec3 oldIndicesU, Eigen::Array4f& oldCoords, const DataArray<T>& sourceArray,
                                    std::vector<AccumulationValueType<T>>& pValues, Eigen::Vector3f& uvw, Point3Df& hitVoxelCenterPoint)
{
  const std::array<Vector3i64, 8>& indexOffset = k_AllOctantOffsets[octant];

  const Vector3i64 oldIndices(static_cast<int64_t>(oldIndicesU[0]), static_cast<int64_t>(oldIndicesU[1]), static_cast<int64_t>(oldIndicesU[2]));
  size_t numComps = sourceArray.getNumberOfComponents();

  Eigen::Vector3f p1Coord;

  for(size_t i = 0; i < 8; i++)
  {
    auto pIndices = oldIndices + indexOffset[i];
    for(size_t compIndex = 0; compIndex < numComps; compIndex++)
    {
      T value = GetSourceArrayValue<T>(params, pIndices, sourceArray, compIndex);
      pValues[i * numComps + compIndex] = value;
    }
    if(i == 0)
    {
      p1Coord = {static_cast<float32>(pIndices[0]) * params.xRes + (0.5F * params.xRes) + params.OriginalOrigin[0],
                 static_cast<float32>(pIndices[1]) * params.yRes + (0.5F * params.yRes) + params.OriginalOrigin[1],
                 static_cast<float32>(pIndices[2]) * params.zRes + (0.5F * params.zRes) + params.OriginalOrigin[2]};
    }
  }

  // NEED TO CALCULATE NEW UVW VALUES BASED ON coordsOld (which is the actual xyz point coord that we need to interpolate).
  auto c000_Index = oldIndices + indexOffset[0];
  auto c111_Index = oldIndices + indexOffset[6];
  Eigen::Vector3f c000_Coord = {static_cast<float32>(c000_Index[0]) * params.xRes + (0.5F * params.xRes) + params.OriginalOrigin[0],
                                static_cast<float32>(c000_Index[1]) * params.yRes + (0.5F * params.yRes) + params.OriginalOrigin[1],
                                static_cast<float32>(c000_Index[2]) * params.zRes + (0.5F * params.zRes) + params.OriginalOrigin[2]};
  Eigen::Vector3f c111_Coord = {static_cast<float32>(c111_Index[0]) * params.xRes + (0.5F * params.xRes) + params.OriginalOrigin[0],
                                static_cast<float32>(c111_Index[1]) * params.yRes + (0.5F * params.yRes) + params.OriginalOrigin[1],
                                static_cast<float32>(c111_Index[2]) * params.zRes + (0.5F * params.zRes) + params.OriginalOrigin[2]};

  for(size_t i = 0; i < 3; i++)
  {
    uvw[i] = (oldCoords[i] - c000_Coord[i]) / (c111_Coord[i] - c000_Coord[i]);
    uvw[i] = uvw[i] < 0.0 ? 0.0 : uvw[i];
    uvw[i] = uvw[i] > 1.0 ? 1.0 : uvw[i];
  }
}

/**
 * @brief The RotateImageGeometryWithTrilinearInterpolation class
 */
template <typename T>
class RotateImageGeometryWithTrilinearInterpolation
{
public:
  RotateImageGeometryWithTrilinearInterpolation(const IDataArray* sourceArray, IDataArray* targetArray, const RotateArgs& rotateArgs, const Matrix4fR& transformationMatrix,
                                                const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  : m_SourceArray(sourceArray)
  , m_TargetArray(targetArray)
  , m_Params(rotateArgs)
  , m_TransformationMatrix(transformationMatrix)
  , m_MessageHandler(messageHandler)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~RotateImageGeometryWithTrilinearInterpolation() = default;

  RotateImageGeometryWithTrilinearInterpolation(const RotateImageGeometryWithTrilinearInterpolation&) = default;

  RotateImageGeometryWithTrilinearInterpolation(RotateImageGeometryWithTrilinearInterpolation&&) noexcept = default;

  RotateImageGeometryWithTrilinearInterpolation& operator=(const RotateImageGeometryWithTrilinearInterpolation&) = delete;

  RotateImageGeometryWithTrilinearInterpolation& operator=(RotateImageGeometryWithTrilinearInterpolation&&) noexcept = delete;

  /**
   * @brief calculateInterpolatedValue
   *
   * This comes from https://en.wikipedia.org/wiki/Trilinear_interpolation
   *
   * Note in the codes below the equations have been changed to do all of the additions first, then
   * the subtractions. This should hopefully alleviate issue with trying to subtract unsigned integers
   * and ending up with what should have been a negative number but since it is unsigned the value
   * that the compiler will compute would be vastly different.
   *
   * @param sourceArray
   * @param oldIndex
   * @param indices
   * @return
   */
  T calculateInterpolatedValue(const std::vector<AccumulationValueType<T>>& pValues, const Eigen::Vector3f& uvw, size_t numComps, size_t compIndex) const
  {
    constexpr size_t P1 = 0;
    constexpr size_t P2 = 1;
    constexpr size_t P3 = 2;
    constexpr size_t P4 = 3;
    constexpr size_t P5 = 4;
    constexpr size_t P6 = 5;
    constexpr size_t P7 = 6;
    constexpr size_t P8 = 7;

    /* clang-format on */
    const AccumulationValueType<T> c000 = pValues[P1 * numComps + compIndex];
    const AccumulationValueType<T> c100 = pValues[P2 * numComps + compIndex];
    const AccumulationValueType<T> c110 = pValues[P3 * numComps + compIndex];
    const AccumulationValueType<T> c010 = pValues[P4 * numComps + compIndex];
    const AccumulationValueType<T> c001 = pValues[P5 * numComps + compIndex];
    const AccumulationValueType<T> c101 = pValues[P6 * numComps + compIndex];
    const AccumulationValueType<T> c111 = pValues[P7 * numComps + compIndex];
    const AccumulationValueType<T> c011 = pValues[P8 * numComps + compIndex];

    const float Xd = uvw[0];
    const float Yd = uvw[1];
    const float Zd = uvw[2];

    const AccumulationValueType<T> c00 = c000 * (1 - Xd) + c100 * Xd;
    const AccumulationValueType<T> c01 = c001 * (1 - Xd) + c101 * Xd;
    const AccumulationValueType<T> c10 = c010 * (1 - Xd) + c110 * Xd;
    const AccumulationValueType<T> c11 = c011 * (1 - Xd) + c111 * Xd;

    const AccumulationValueType<T> c0 = c00 * (1 - Yd) + c10 * Yd;
    const AccumulationValueType<T> c1 = c01 * (1 - Yd) + c11 * Yd;

    const AccumulationValueType<T> c = c0 * (1 - Zd) + c1 * Zd;

    return c;
  }

  /**
   * @brief This is the main algorithm to perform the interpolation and get a final value that is placed into the transformed
   * voxel. This uses Trilinear interpolation which will devolve into Bilinear and Linear interpolation depending on the
   * values of U, V and W.
   */
  void operator()() const
  {
    using DataArrayType = DataArray<T>;

    const auto& sourceArray = dynamic_cast<const DataArrayType&>(*m_SourceArray);
    const size_t numComps = sourceArray.getNumberOfComponents();
    if(numComps == 0)
    {
      m_MessageHandler(fmt::format("{}: Number of Components was Zero for array. Exiting Transform.", sourceArray.getName()));
      return;
    }

    m_MessageHandler(fmt::format("{}: Transform Starting", sourceArray.getName()));

    FilterMessenger filterMessenger(m_MessageHandler);
    const auto arrayName = m_SourceArray->getName();
    const int64_t totalSlices = m_Params.outputDims[2];
    filterMessenger.setThrottledFormatter([arrayName, totalSlices](usize slice) { return fmt::format("{}: Interpolating values for slice '{}/{}'", arrayName, slice, totalSlices); });

    auto& newDataStore = m_TargetArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    DataStructure tempDataStructure;
    ImageGeom* origImageGeomPtr = ImageGeom::Create(tempDataStructure, "Temp");
    origImageGeomPtr->setDimensions(m_Params.OriginalDims);
    origImageGeomPtr->setSpacing(m_Params.OriginalSpacing);
    origImageGeomPtr->setOrigin(m_Params.OriginalOrigin);

    ImageGeom* destImageGeomPtr = ImageGeom::Create(tempDataStructure, "dest image geom");
    destImageGeomPtr->setDimensions(m_Params.TransformedDims);
    destImageGeomPtr->setSpacing(m_Params.TransformedSpacing);
    destImageGeomPtr->setOrigin(m_Params.TransformedOrigin);

    std::vector<AccumulationValueType<T>> pValues(8 * numComps);

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();

    for(int64_t k = 0; k < m_Params.outputDims[2]; k++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      filterMessenger.sendThrottledMessage(static_cast<usize>(k));
      int64_t ktot = (m_Params.outputDims[0] * m_Params.outputDims[1]) * k;

      for(int64_t j = 0; j < m_Params.outputDims[1]; j++)
      {
        int64_t jtot = (m_Params.outputDims[0]) * j;
        for(int64_t i = 0; i < m_Params.outputDims[0]; i++)
        {
          int64_t destIndex = ktot + jtot + i;
          Point3Df destPoint = destImageGeomPtr->getCoordsf(destIndex);
          // Last value is 1. See https://www.euclideanspace.com/maths/geometry/affine/matrix4x4/index.htm
          Eigen::Vector4f coordsNew(destPoint.getX(), destPoint.getY(), destPoint.getZ(), 1.0f);
          // Transform back to the old coordinate
          Eigen::Array4f coordsOld = inverseTransform * coordsNew;

          // Now compute the old Cell Index from the old coordinate
          SizeVec3 oldGeomIndices;
          auto errorResult = origImageGeomPtr->computeCellIndex(coordsOld.data(), oldGeomIndices);

          // Now we know what voxel the new cell center maps back to in the original geometry.
          if(errorResult == ImageGeom::ErrorType::NoError)
          {
            size_t oldIndex = (m_Params.OriginalDims[0] * m_Params.OriginalDims[1] * oldGeomIndices[2]) + (m_Params.OriginalDims[0] * oldGeomIndices[1]) + oldGeomIndices[0];

            auto oldVoxelCenterPoint = origImageGeomPtr->getCoordsf(oldIndex);

            int octant = FindOctant(m_Params, oldVoxelCenterPoint, coordsOld);

            Eigen::Vector3f uvw;
            FindInterpolationValues(m_Params, octant, oldGeomIndices, coordsOld, sourceArray, pValues, uvw, oldVoxelCenterPoint);

            for(size_t compIndex = 0; compIndex < numComps; compIndex++)
            {
              T value = calculateInterpolatedValue(pValues, uvw, numComps, compIndex);
              newDataStore.setComponent(destIndex, compIndex, value);
            }
          }
          else
          {
            newDataStore.fillTuple(destIndex, static_cast<T>(0));
          }
        }
      }
    }
    m_MessageHandler(fmt::format("{}: Transform Ending", sourceArray.getName()));
  }

private:
  const IDataArray* m_SourceArray;
  IDataArray* m_TargetArray;
  ImageRotationUtilities::RotateArgs m_Params;
  Matrix4fR m_TransformationMatrix;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
};

//------------------------------------------------------------------------------
template <typename T>
class RotateImageGeometryWithNearestNeighbor
{
public:
  RotateImageGeometryWithNearestNeighbor(const IDataArray* sourceArray, IDataArray* targetArray, const RotateArgs& args, const Matrix4fR& transformationMatrix, bool sliceBySlice,
                                         const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  : m_SourceArray(sourceArray)
  , m_TargetArray(targetArray)
  , m_Params(args)
  , m_TransformationMatrix(transformationMatrix)
  , m_SliceBySlice(sliceBySlice)
  , m_MessageHandler(messageHandler)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~RotateImageGeometryWithNearestNeighbor() = default;

  RotateImageGeometryWithNearestNeighbor(const RotateImageGeometryWithNearestNeighbor&) = default;

  RotateImageGeometryWithNearestNeighbor(RotateImageGeometryWithNearestNeighbor&&) noexcept = default;

  RotateImageGeometryWithNearestNeighbor& operator=(const RotateImageGeometryWithNearestNeighbor&) = delete;

  RotateImageGeometryWithNearestNeighbor& operator=(RotateImageGeometryWithNearestNeighbor&&) noexcept = delete;

  void convert() const
  {
    FilterMessenger filterMessenger(m_MessageHandler);
    const auto arrayName = m_SourceArray->getName();
    const int64 totalSlices = m_Params.outputDims[2];
    filterMessenger.setThrottledFormatter([arrayName, totalSlices](usize slice) { return fmt::format("{}: Interpolating values for slice '{}/{}'", arrayName, slice, totalSlices); });

    DataStructure tempDataStructure;
    ImageGeom* srcImageGeomPtr = ImageGeom::Create(tempDataStructure, "source image geom");
    srcImageGeomPtr->setDimensions(m_Params.OriginalDims);
    srcImageGeomPtr->setSpacing(m_Params.OriginalSpacing);
    srcImageGeomPtr->setOrigin(m_Params.OriginalOrigin);

    ImageGeom* destImageGeomPtr = ImageGeom::Create(tempDataStructure, "dest image geom");
    destImageGeomPtr->setDimensions(m_Params.TransformedDims);
    destImageGeomPtr->setSpacing(m_Params.TransformedSpacing);
    destImageGeomPtr->setOrigin(m_Params.TransformedOrigin);

    const auto& oldDataStore = m_SourceArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = m_TargetArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();
    for(int64 k = 0; k < m_Params.outputDims[2]; k++)
    {
      if(m_ShouldCancel)
      {
        break;
      }
      filterMessenger.sendThrottledMessage(static_cast<usize>(k));

      int64 const ktot = (m_Params.outputDims[0] * m_Params.outputDims[1]) * k;
      for(int64 j = 0; j < m_Params.outputDims[1]; j++)
      {
        int64 jtot = (m_Params.outputDims[0]) * j;
        for(int64 i = 0; i < m_Params.outputDims[0]; i++)
        {
          const int64 destIndex = ktot + jtot + i;
          Point3Df destPoint = destImageGeomPtr->getCoordsf(destIndex);
          // Last value is 1. See https://www.euclideanspace.com/maths/geometry/affine/matrix4x4/index.htm
          Eigen::Vector4f coordsNew(destPoint.getX(), destPoint.getY(), destPoint.getZ(), 1.0f);
          // Transform back to the old coordinate
          Eigen::Array4f coordsOld = inverseTransform * coordsNew;

          // Now compute the old Cell Index from the old coordinate
          SizeVec3 oldGeomIndices;
          auto errorResult = srcImageGeomPtr->computeCellIndex(coordsOld.data(), oldGeomIndices);

          // Now we know what voxel the new cell center maps back to in the original geometry.
          if(errorResult == ImageGeom::ErrorType::NoError)
          {
            if(m_SliceBySlice)
            {
              oldGeomIndices[2] = k;
            }
            size_t oldIndex = (m_Params.OriginalDims[0] * m_Params.OriginalDims[1] * oldGeomIndices[2]) + (m_Params.OriginalDims[0] * oldGeomIndices[1]) + oldGeomIndices[0];

            if(newDataStore.copyFrom(destIndex, oldDataStore, oldIndex, 1).invalid())
            {
              std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_SourceArray->getName(), oldIndex,
                                       m_SourceArray->getName(), destIndex)
                        << std::endl;
              break;
            }
          }
          else
          {
            newDataStore.fillTuple(destIndex, 0);
          }
        }
      }
    }
    m_MessageHandler(fmt::format("{}: Transform Ending", m_SourceArray->getName()));
  }

  void operator()() const
  {
    convert();
  }

private:
  const IDataArray* m_SourceArray;
  IDataArray* m_TargetArray;
  ImageRotationUtilities::RotateArgs m_Params;
  const Matrix4fR& m_TransformationMatrix;
  bool m_SliceBySlice = false;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief The ApplyTransformationToNodeGeometry class will apply a transformation to a node based geometry.
 */
class ApplyTransformationToNodeGeometry
{
public:
  ApplyTransformationToNodeGeometry(IGeometry::SharedVertexList& verticesPtr, const Matrix4fR& transformationMatrix, ProgressWorker progressWorker, const std::atomic_bool& shouldCancel)
  : m_TransformationMatrix(transformationMatrix)
  , m_Vertices(verticesPtr)
  , m_ProgressWorker(std::move(progressWorker))
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief operator () This is called from the TBB style of code
   * @param range The range to compute the values
   */
  void operator()(const Range& range) const
  {
    for(size_t i = range.min(); i < range.max(); i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const Eigen::Vector4f position(m_Vertices.at(3 * i + 0), m_Vertices.at(3 * i + 1), m_Vertices.at(3 * i + 2), 1);
      Eigen::Vector4f transformedPosition = m_TransformationMatrix * position;
      m_Vertices.setValue(3 * i + 0, transformedPosition[0]);
      m_Vertices.setValue(3 * i + 1, transformedPosition[1]);
      m_Vertices.setValue(3 * i + 2, transformedPosition[2]);

      m_ProgressWorker.incrementProgress(1);
    }
  }

private:
  const Matrix4fR& m_TransformationMatrix;
  IGeometry::SharedVertexList& m_Vertices;
  mutable ProgressWorker m_ProgressWorker;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace nx::core::ImageRotationUtilities
