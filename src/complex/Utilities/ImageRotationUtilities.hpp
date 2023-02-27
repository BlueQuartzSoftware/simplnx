#pragma once

#include "complex/Common/Array.hpp"
#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/complex_export.hpp"

#include <Eigen/Dense>

#include <fstream>
#include <iostream>

namespace complex::ImageRotationUtilities
{
const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();

using Matrix3fR = Eigen::Matrix<float, 3, 3, Eigen::RowMajor>;
using Matrix4fR = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

using Transform3f = Eigen::Transform<float, 3, Eigen::Affine>;
using MatrixTranslation = Eigen::Matrix<float, 1, 3, Eigen::RowMajor>;

using Vector3s = Eigen::Array<size_t, 1, 3>;
using Vector3i64 = Eigen::Array<int64_t, 1, 3>;

using Int64Vec3Type = complex::Vec3<int64_t>;

struct RotateArgs
{

  USizeVec3 originalDims;
  FloatVec3 originalSpacing;
  FloatVec3 originalOrigin;
  int64_t xp = 0;
  int64_t yp = 0;
  int64_t zp = 0;
  float xRes = 0.0f;
  float yRes = 0.0f;
  float zRes = 0.0f;

  USizeVec3 transformedDims;
  FloatVec3 transformedSpacing;
  FloatVec3 transformedOrigin;
  int64_t xpNew = 0;
  int64_t ypNew = 0;
  int64_t zpNew = 0;
  float xResNew = 0.0f;
  float yResNew = 0.0f;
  float zResNew = 0.0f;
  float xMinNew = 0.0f;
  float yMinNew = 0.0f;
  float zMinNew = 0.0f;
};

/**
 * @brief
 * @param transformationMatrix
 * @return
 */
COMPLEX_EXPORT std::string GenerateTransformationMatrixDescription(const ImageRotationUtilities::Matrix4fR& transformationMatrix);

/**
 * @brief
 * @param precomputed
 * @return
 */
COMPLEX_EXPORT Matrix4fR CopyPrecomputedToTransformationMatrix(const Float32Array& precomputed);

/**
 * @brief
 * @param tableData
 * @return
 */
COMPLEX_EXPORT Matrix4fR GenerateManualTransformationMatrix(const DynamicTableParameter::ValueType& tableData);

/**
 * @brief
 * @param pRotationValue
 * @return
 */
COMPLEX_EXPORT Matrix4fR GenerateRotationTransformationMatrix(const VectorFloat32Parameter::ValueType& pRotationValue);

/**
 * @brief
 * @param pTranslationValue
 * @return
 */
COMPLEX_EXPORT Matrix4fR GenerateTranslationTransformationMatrix(const VectorFloat32Parameter::ValueType& pTranslationValue);
/**
 * @brief
 * @param pScaleValue
 * @return
 */
COMPLEX_EXPORT Matrix4fR GenerateScaleTransformationMatrix(const VectorFloat32Parameter::ValueType& pScaleValue);

/**
 * @brief Function to determine the min and max coordinates (bounding box) of the transformed Image Geometry.
 * @param imageGeometry
 * @param transformationMatrix
 * @return
 */
COMPLEX_EXPORT FloatVec6 DetermineMinMaxCoords(const ImageGeom& imageGeometry, const Matrix4fR& transformationMatrix);

/**
 * @brief Finds the Cosine of the angle between 2 vectors
 * @param a
 * @param b
 * @return
 */
COMPLEX_EXPORT float CosBetweenVectors(const Eigen::Vector3f& a, const Eigen::Vector3f& b);

/**
 * @brief Function for determining new ImageGeom Spacing between points for scaling
 * @param spacing
 * @param axisNew
 * @return spacing for a given axis.
 */
COMPLEX_EXPORT float DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew);

/**
 * @brief Determines parameters for image rotation
 * @param imageGeom
 * @param transformationMatrix
 * @return New RotateArgs object
 */
COMPLEX_EXPORT ImageRotationUtilities::RotateArgs CreateRotationArgs(const ImageGeom& imageGeom, const Matrix4fR& transformationMatrix);

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
  size_t index = (xyzIndex[2] * params.xp * params.yp) + (xyzIndex[1] * params.xp) + xyzIndex[0];
  return sourceArray[index * sourceArray.getNumberOfComponents() + compIndex];
}

/**
 * @brief
 * @param m_Dimensions
 * @param m_Spacing
 * @param m_Origin
 * @param idx
 * @return
 */
// COMPLEX_EXPORT Point3D<float32> GetCoords(const USizeVec3& m_Dimensions, const FloatVec3& m_Spacing, const FloatVec3& m_Origin, usize idx);

template <typename ArrayType>
ArrayType GetCoords(const USizeVec3& m_Dimensions, const FloatVec3& m_Spacing, const FloatVec3& m_Origin, usize idx)
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  ArrayType coords;
  coords[0] = column * m_Spacing[0] + m_Origin[0] + (0.5f * m_Spacing[0]);
  coords[1] = row * m_Spacing[1] + m_Origin[1] + (0.5f * m_Spacing[1]);
  coords[2] = plane * m_Spacing[2] + m_Origin[2] + (0.5f * m_Spacing[2]);
  return coords;
}

template <typename ArrayType>
std::optional<SizeVec3> ComputeCellIndex(const USizeVec3& m_Dimensions, const FloatVec3& m_Spacing, const FloatVec3& m_Origin, const ArrayType& coords)
{
  SizeVec3 index(0, 0, 0);

  for(usize i = 0; i < 3; i++)
  {
    if(coords[i] < m_Origin[i])
    {
      return {};
    }
    if(coords[i] > (m_Origin[i] + m_Dimensions[i] * m_Spacing[i]))
    {
      return {};
    }
    index[i] = static_cast<usize>((coords[i] - m_Origin[i]) / m_Spacing[i]);
    if(index[i] > m_Dimensions[i])
    {
      return {};
    }
  }
  return index;
}

/**
 * @brief FindOctant
 * @param params
 * @param index
 * @param coord
 * @return
 */
COMPLEX_EXPORT size_t FindOctant(const RotateArgs& params, size_t index, FloatVec3 coord);

using OctantOffsetArrayType = std::array<Vector3i64, 8>;

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

/**
 * @brief FindInterpolationValues
 * @param params
 * @param index
 * @param octant
 * @param oldIndicesU
 * @param oldCoords
 * @param sourceArray
 * @param pValues
 * @param uvw
 */
template <typename T>
inline void FindInterpolationValues(const RotateArgs& params, size_t index, size_t octant, SizeVec3 oldIndicesU, Eigen::Array4f& oldCoords, const DataArray<T>& sourceArray, std::vector<T>& pValues,
                                    Eigen::Vector3f& uvw)
{
  const std::array<Vector3i64, 8>& indexOffset = k_AllOctantOffsets[octant];

  Vector3i64 oldIndices(static_cast<int64_t>(oldIndicesU[0]), static_cast<int64_t>(oldIndicesU[1]), static_cast<int64_t>(oldIndicesU[2]));
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
      p1Coord = {pIndices[0] * params.xRes + (0.5F * params.xRes) + params.originalOrigin[0], pIndices[1] * params.yRes + (0.5F * params.yRes) + params.originalOrigin[1],
                 pIndices[2] * params.zRes + (0.5F * params.zRes) + params.originalOrigin[2]};
    }
  }
  uvw[0] = oldCoords[0] - p1Coord[0];
  uvw[1] = oldCoords[1] - p1Coord[1];
  uvw[2] = oldCoords[2] - p1Coord[2];
}

/**
 * @brief The RotateImageGeometryWithTrilinearInterpolation class
 */
template <typename T>
class RotateImageGeometryWithTrilinearInterpolation
{
private:
  const IDataArray* m_SourceArray;
  IDataArray* m_TargetArray;
  ImageRotationUtilities::RotateArgs m_Params;
  Matrix4fR m_TransformationMatrix;

public:
  RotateImageGeometryWithTrilinearInterpolation(const IDataArray* sourceArray, IDataArray* targetArray, const RotateArgs& rotateArgs, const Matrix4fR transformationMatrix)
  : m_SourceArray(sourceArray)
  , m_TargetArray(targetArray)
  , m_Params(rotateArgs)
  , m_TransformationMatrix(std::move(transformationMatrix))
  {
  }

  /**
   * @brief CalculateInterpolatedValue
   *
   * This comes from https://www.cs.purdue.edu/homes/cs530/slides/04.DataStructure.pdf, page 36.
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
  T CalculateInterpolatedValue(std::vector<T>& pValues, Eigen::Vector3f& uvw, size_t numComps, size_t compIndex) const
  {
    constexpr size_t P1 = 0;
    constexpr size_t P2 = 1;
    constexpr size_t P3 = 2;
    constexpr size_t P4 = 3;
    constexpr size_t P5 = 4;
    constexpr size_t P6 = 5;
    constexpr size_t P7 = 6;
    constexpr size_t P8 = 7;

    const float u = uvw[0];
    const float v = uvw[1];
    const float w = uvw[2];

    T value = pValues[0];
    // clang-format off
    value += u * (pValues[P2 * numComps + compIndex] - pValues[P1 * numComps + compIndex]);
    value += v * (pValues[P4 * numComps + compIndex] - pValues[P1 * numComps + compIndex]);
    value += w * (pValues[P5 * numComps + compIndex] - pValues[P1 * numComps + compIndex]);
    value += u * v * (pValues[P1 * numComps + compIndex] + pValues[P3 * numComps + compIndex] - pValues[P2 * numComps + compIndex] - pValues[P4 * numComps + compIndex]);
    value += u * w * (pValues[P1 * numComps + compIndex] + pValues[P6 * numComps + compIndex] - pValues[P2 * numComps + compIndex] - pValues[P5 * numComps + compIndex]);
    value += v * w * (pValues[P1 * numComps + compIndex] + pValues[P8 * numComps + compIndex] - pValues[P4 * numComps + compIndex] - pValues[P5 * numComps + compIndex]);
    value += u * v * w *
             ( pValues[P4 * numComps + compIndex]
              + pValues[P2 * numComps + compIndex]
              + pValues[P8 * numComps + compIndex]
              + pValues[P6 * numComps + compIndex]
              - pValues[P1 * numComps + compIndex]
              - pValues[P3 * numComps + compIndex]
              - pValues[P5 * numComps + compIndex]
              - pValues[P7 * numComps + compIndex] );

    // clang-format on
    return value;
  }

  /**
   * @brief This is the main algorithm to perform the interpolation and get a final value that is placed into the transformed
   * voxel. This uses Trilinear interpolation which will devolve into Bilinear and Linear interpolation depending on the
   * values of U, V and W.
   */
  void operator()() const
  {
    using DataArrayType = DataArray<T>;

    const DataArrayType& sourceArray = dynamic_cast<const DataArrayType&>(*m_SourceArray);
    const size_t numComps = sourceArray.getNumberOfComponents();
    //    if(numComps == 0)
    //    {
    //      m_Filter->setErrorCondition(-1000, "Invalid DataArray with ZERO components");
    //      m_Filter->sendThreadSafeProgressMessage(fmt::format("{}: Number of Components was Zero for array. Exiting Transform.", sourceArray.getName()));
    //      return;
    //    }
    //
    //    m_Filter->sendThreadSafeProgressMessage(fmt::format("{}: Transform Starting", sourceArray.getName()));

    const auto& oldDataStore = m_SourceArray->getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = m_TargetArray->getIDataStoreRefAs<AbstractDataStore<T>>();

    DataArrayType& targetArray = dynamic_cast<DataArrayType&>(*m_TargetArray);

    DataStructure tempDataStructure;
    ImageGeom* origImageGeom = ImageGeom::Create(tempDataStructure, "Temp");
    origImageGeom->setDimensions(m_Params.originalDims);
    origImageGeom->setSpacing(m_Params.originalSpacing);
    origImageGeom->setOrigin(m_Params.originalOrigin);

    ImageGeom* destImageGeom = ImageGeom::Create(tempDataStructure, "dest image geom");
    destImageGeom->setDimensions(m_Params.transformedDims);
    destImageGeom->setSpacing(m_Params.transformedSpacing);
    destImageGeom->setOrigin(m_Params.transformedOrigin);

    // Tri linearInterpolationData<T> interpolationValues;
    // SizeVec3 oldGeomIndices = {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};

    Eigen::Vector4f coordsNew;
    std::vector<T> pValues(8 * numComps);
    Eigen::Vector3f uvw;

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();

    for(int64_t k = 0; k < m_Params.zpNew; k++)
    {
      //      if(m_Filter->getCancel() || m_Filter->getErrorCode() < 0)
      //      {
      //        break;
      //      }
      int64_t ktot = (m_Params.xpNew * m_Params.ypNew) * k;

      //      m_Filter->sendThreadSafeProgressMessage(fmt::format("{}: Interpolating values for slice '{}/{}'", m_SourceArray->getName(), k, m_Params.zpNew));
      for(int64_t j = 0; j < m_Params.ypNew; j++)
      {
        int64_t jtot = (m_Params.xpNew) * j;
        for(int64_t i = 0; i < m_Params.xpNew; i++)
        {
          int64_t newIndex = ktot + jtot + i;

          coordsNew = Eigen::Vector4f(destImageGeom->getCoordsf(newIndex).toArray().data());
          // Transform back to the old coordinate
          Eigen::Array4f coordsOld = inverseTransform * coordsNew;

          // Now compute the old Cell Index from the old coordinate
          SizeVec3 oldGeomIndices;
          auto errorResult = origImageGeom->computeCellIndex(coordsOld.data(), oldGeomIndices);

          // Now we know what voxel the new cell center maps back to in the original geometry.
          if(errorResult == ImageGeom::ErrorType::NoError)
          {
            size_t oldIndex = (m_Params.originalDims[0] * m_Params.originalDims[1] * oldGeomIndices[2]) + (m_Params.originalDims[0] * m_Params.originalDims[1]) + oldGeomIndices[0];
            int octant = FindOctant(m_Params, oldIndex, {coordsOld.data()});

            FindInterpolationValues(m_Params, oldIndex, octant, oldGeomIndices, coordsOld, sourceArray, pValues, uvw);
            for(size_t compIndex = 0; compIndex < numComps; compIndex++)
            {
              T value = CalculateInterpolatedValue(pValues, uvw, numComps, compIndex);
              newDataStore.setComponent(newIndex, compIndex, value);
            }
          }
          else
          {
            newDataStore.fillTuple(newIndex, static_cast<T>(0));
          }
        }
      }
    }
    // m_Filter->sendThreadSafeProgressMessage(fmt::format("{}: Transform Ending", sourceArray.getName()));
  }
};

//------------------------------------------------------------------------------
template <typename T>
class RotateImageGeometryWithNearestNeighbor
{
private:
  const IDataArray* m_SourceArray;
  IDataArray* m_TargetArray;
  ImageRotationUtilities::RotateArgs m_Params;
  const Matrix4fR& m_TransformationMatrix;
  bool m_SliceBySlice = false;

public:
  RotateImageGeometryWithNearestNeighbor(const IDataArray* sourceArray, IDataArray* targetArray, const RotateArgs& args, const Matrix4fR transformationMatrix, bool sliceBySlice)
  : m_SourceArray(sourceArray)
  , m_TargetArray(targetArray)
  , m_Params(args)
  , m_TransformationMatrix(std::move(transformationMatrix))
  , m_SliceBySlice(sliceBySlice)
  {
  }
  ~RotateImageGeometryWithNearestNeighbor() = default;

  RotateImageGeometryWithNearestNeighbor(const RotateImageGeometryWithNearestNeighbor&) = default;
  RotateImageGeometryWithNearestNeighbor(RotateImageGeometryWithNearestNeighbor&&) noexcept = default;
  RotateImageGeometryWithNearestNeighbor& operator=(const RotateImageGeometryWithNearestNeighbor&) = delete;
  RotateImageGeometryWithNearestNeighbor& operator=(RotateImageGeometryWithNearestNeighbor&&) noexcept = delete;

  void convert() const
  {

    DataStructure tempDataStructure;
    ImageGeom* srcImageGeom = ImageGeom::Create(tempDataStructure, "source image geom");
    srcImageGeom->setDimensions(m_Params.originalDims);
    srcImageGeom->setSpacing(m_Params.originalSpacing);
    srcImageGeom->setOrigin(m_Params.originalOrigin);

    ImageGeom* destImageGeom = ImageGeom::Create(tempDataStructure, "dest image geom");
    destImageGeom->setDimensions(m_Params.transformedDims);
    destImageGeom->setSpacing(m_Params.transformedSpacing);
    destImageGeom->setOrigin(m_Params.transformedOrigin);

    const auto& oldDataStore = m_SourceArray->getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = m_TargetArray->getIDataStoreRefAs<AbstractDataStore<T>>();

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();
    Eigen::Vector4f coordsNew;
    coordsNew[3] = 0.0F;
    Eigen::Array4f coordsOld;
    for(int64 k = 0; k < m_Params.zpNew; k++)
    {
      int64 const ktot = (m_Params.xpNew * m_Params.ypNew) * k;
      for(int64 j = 0; j < m_Params.ypNew; j++)
      {
        int64 jtot = (m_Params.xpNew) * j;
        for(int64 i = 0; i < m_Params.xpNew; i++)
        {
          int64 const newIndex = ktot + jtot + i;
          coordsNew = Eigen::Vector4f(destImageGeom->getCoordsf(newIndex).toArray().data());
          // Transform back to the old coordinate
          coordsOld = inverseTransform * coordsNew;

          // Now compute the old Cell Index from the old coordinate
          SizeVec3 oldGeomIndices;
          auto errorResult = srcImageGeom->computeCellIndex(coordsOld.data(), oldGeomIndices);

          // Now we know what voxel the new cell center maps back to in the original geometry.
          if(errorResult == ImageGeom::ErrorType::NoError)
          {
            if(m_SliceBySlice)
            {
              oldGeomIndices[2] = k;
            }
            size_t oldIndex = (m_Params.originalDims[0] * m_Params.originalDims[1] * oldGeomIndices[2]) + (m_Params.originalDims[0] * oldGeomIndices[1]) + oldGeomIndices[0];

            if(!newDataStore.copyFrom(newIndex, oldDataStore, oldIndex, 1))
            {
              std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_SourceArray->getName(), oldIndex,
                                       m_SourceArray->getName(), newIndex)
                        << std::endl;
              break;
            }
          }
          else
          {
            newDataStore.fillTuple(newIndex, 0);
          }
        }
      }
    }
  }

  void operator()() const
  {
    convert();
  }
};

#if 0
/**
 * @brief ExecuteParallelFunction
 * @param sourceArray
 * @param runner
 * @param args
 */
template <template <class, class> class ClassT, class ParallelRunnerT, class FilterType, class... ArgsT>
auto ExecuteParallelFunction(IDataArray*& sourceArray, ParallelRunnerT&& runner, FilterType* filter, ArgsT&&... args)
{

  if(DataArray<int8_t>* array = std::dynamic_pointer_cast<DataArray<int8_t>>(sourceArray))
  {
    return runner.template execute<>(ClassT<int8_t, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<int16_t>* array = std::dynamic_pointer_cast<DataArray<int16_t>>(sourceArray))
  {
    return runner.template execute<>(ClassT<int16_t, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<int32_t>* array = std::dynamic_pointer_cast<DataArray<int32_t>>(sourceArray))
  {
    return runner.template execute<>(ClassT<int32_t, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<int64_t>* array = std::dynamic_pointer_cast<DataArray<int64_t>>(sourceArray))
  {
    return runner.template execute<>(ClassT<int64_t, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<uint8_t>* array = std::dynamic_pointer_cast<DataArray<uint8_t>>(sourceArray))
  {
    return runner.template execute<>(ClassT<uint8_t, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<uint16_t>* array = std::dynamic_pointer_cast<DataArray<uint16_t>>(sourceArray))
  {
    return runner.template execute<>(ClassT<uint16_t, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<uint32_t>* array = std::dynamic_pointer_cast<DataArray<uint32_t>>(sourceArray))
  {
    return runner.template execute<>(ClassT<uint32_t, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<uint64_t>* array = std::dynamic_pointer_cast<DataArray<uint64_t>>(sourceArray))
  {
    return runner.template execute<>(ClassT<uint64_t, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<float>* array = std::dynamic_pointer_cast<DataArray<float>>(sourceArray))
  {
    return runner.template execute<>(ClassT<float, FilterType>(std::forward<ArgsT>(args)...));
  }
  if(DataArray<double>* array = std::dynamic_pointer_cast<DataArray<double>>(sourceArray))
  {
    return runner.template execute<>(ClassT<double, FilterType>(std::forward<ArgsT>(args)...));
  }

  throw std::runtime_error("Can not interpolate data type. Bool, String, StatsData?");
}
#endif
} // namespace complex::ImageRotationUtilities
