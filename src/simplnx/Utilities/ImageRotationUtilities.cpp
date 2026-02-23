#include "ImageRotationUtilities.hpp"

#include <algorithm>

namespace nx::core::ImageRotationUtilities
{
//------------------------------------------------------------------------------
FloatVec6 DetermineMinMaxCoords(const BoundingBox3Df& imageGeomBoundingBox, const Matrix4fR& transformationMatrix)
{
  auto min = imageGeomBoundingBox.getMinPoint();
  auto max = imageGeomBoundingBox.getMaxPoint();
  /* clang-format off */
  std::vector<FloatVec3> imageGeomCornerCoords = {{min[0], min[1], min[2]},
                                                  {min[0], min[1], max[2]},
                                                  {min[0], max[1], min[2]},
                                                  {min[0], max[1], max[2]},
                                                  {max[0], min[1], min[2]},
                                                  {max[0], min[1], max[2]},
                                                  {max[0], max[1], min[2]},
                                                  {max[0], max[1], max[2]}};

  /* clang-format on */
  FloatVec6 minMaxValues = {std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                            -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max()};

  for(size_t i = 0; i < 8; i++)
  {
    const Eigen::Vector4f coords(imageGeomCornerCoords[i][0], imageGeomCornerCoords[i][1], imageGeomCornerCoords[i][2], 1.0F);

    Eigen::Vector4f newCoords = transformationMatrix * coords;

    minMaxValues[0] = std::min(newCoords[0], minMaxValues[0]);
    minMaxValues[1] = std::max(newCoords[0], minMaxValues[1]);

    minMaxValues[2] = std::min(newCoords[1], minMaxValues[2]);
    minMaxValues[3] = std::max(newCoords[1], minMaxValues[3]);

    minMaxValues[4] = std::min(newCoords[2], minMaxValues[4]);
    minMaxValues[5] = std::max(newCoords[2], minMaxValues[5]);
  }
  return minMaxValues;
}

//------------------------------------------------------------------------------
FloatVec6 DetermineMinMaxCoords(const ImageGeom& imageGeometry, const Matrix4fR& transformationMatrix)
{
  auto origImageGeomBox = imageGeometry.getBoundingBoxf();
  return DetermineMinMaxCoords(origImageGeomBox, transformationMatrix);
}

//------------------------------------------------------------------------------
float DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew)
{
  const float xAngle = std::abs(CosBetweenVectors(k_XAxis, axisNew));
  const float yAngle = std::abs(CosBetweenVectors(k_YAxis, axisNew));
  const float zAngle = std::abs(CosBetweenVectors(k_ZAxis, axisNew));

  const std::array<float, 3> axes = {xAngle, yAngle, zAngle};

  const auto maxElementIterPtr = std::max_element(axes.cbegin(), axes.cend());

  const size_t index = std::distance(axes.cbegin(), maxElementIterPtr);

  return spacing[index];
}

//------------------------------------------------------------------------------
RotateArgs CreateRotationArgs(const ImageGeom& imageGeom, const Matrix4fR& transformationMatrix)
{
  const SizeVec3 origDims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();

  const Matrix3fR rotationMatrix = transformationMatrix.block(0, 0, 3, 3);

  FloatVec6 minMaxCoords = DetermineMinMaxCoords(imageGeom, transformationMatrix);

  const Eigen::Vector3f xAxisNew = rotationMatrix * k_XAxis;
  const Eigen::Vector3f yAxisNew = rotationMatrix * k_YAxis;
  const Eigen::Vector3f zAxisNew = rotationMatrix * k_ZAxis;

  FloatVec3 transformScale = {(rotationMatrix * k_XAxis).norm(), (rotationMatrix * k_YAxis).norm(), (rotationMatrix * k_ZAxis).norm()};

  FloatVec3 outputSpacing = {DetermineSpacing(spacing, xAxisNew) * transformScale[0], DetermineSpacing(spacing, yAxisNew) * transformScale[1], DetermineSpacing(spacing, zAxisNew) * transformScale[2]};

  USizeVec3 outputDims = {static_cast<IGeometry::MeshIndexType>(std::nearbyint((minMaxCoords[1] - minMaxCoords[0]) / outputSpacing[0])),
                          static_cast<IGeometry::MeshIndexType>(std::nearbyint((minMaxCoords[3] - minMaxCoords[2]) / outputSpacing[1])),
                          static_cast<IGeometry::MeshIndexType>(std::nearbyint((minMaxCoords[5] - minMaxCoords[4]) / outputSpacing[2]))};

  if(outputDims[0] == 0)
  {
    outputDims[0] = static_cast<IGeometry::MeshIndexType>(1);
  }
  if(outputDims[1] == 0)
  {
    outputDims[1] = static_cast<IGeometry::MeshIndexType>(1);
  }
  if(outputDims[2] == 0)
  {
    outputDims[2] = static_cast<IGeometry::MeshIndexType>(1);
  }

  RotateArgs params;

  params.OriginalDims = imageGeom.getDimensions();
  params.OriginalSpacing = imageGeom.getSpacing();
  params.OriginalOrigin = imageGeom.getOrigin();

  params.xp = static_cast<int64>(origDims[0]);
  params.xRes = spacing[0];
  params.yp = static_cast<int64>(origDims[1]);
  params.yRes = spacing[1];
  params.zp = static_cast<int64>(origDims[2]);
  params.zRes = spacing[2];

  params.TransformedDims = outputDims;       //{xpNew, ypNew, zpNew};
  params.TransformedSpacing = outputSpacing; //{xResNew, yResNew, zResNew};
  params.TransformedOrigin = {minMaxCoords[0], minMaxCoords[2], minMaxCoords[4]};

  params.outputDims = outputDims;
  params.outputSpacing = outputSpacing;

  params.outputXMin = minMaxCoords[0];
  params.outputYMin = minMaxCoords[2];
  params.outputZMin = minMaxCoords[4];

  return params;
}

//------------------------------------------------------------------------------
std::string GenerateTransformationMatrixDescription(const Matrix4fR& transform)
{
  std::stringstream out;

  out << fmt::format("| {:+f}  {:+f}  {:+f}  {:+f} |", transform(0, 0), transform(0, 1), transform(0, 2), transform(0, 3)) << "\n"
      << fmt::format("| {:+f}  {:+f}  {:+f}  {:+f} |", transform(1, 0), transform(1, 1), transform(1, 2), transform(1, 3)) << "\n"
      << fmt::format("| {:+f}  {:+f}  {:+f}  {:+f} |", transform(2, 0), transform(2, 1), transform(2, 2), transform(2, 3)) << "\n"
      << fmt::format("| {:+f}  {:+f}  {:+f}  {:+f} |", transform(3, 0), transform(3, 1), transform(3, 2), transform(3, 3)) << std::endl;

  return out.str();
}

//------------------------------------------------------------------------------
Matrix4fR CopyPrecomputedToTransformationMatrix(const AbstractDataStore<float32>& precomputed)
{
  Matrix4fR transformationMatrix;
  transformationMatrix.fill(0.0F);

  for(int64_t rowIndex = 0; rowIndex < 4; rowIndex++)
  {
    for(int64_t colIndex = 0; colIndex < 4; colIndex++)
    {
      transformationMatrix(rowIndex, colIndex) = precomputed[4 * rowIndex + colIndex];
    }
  }
  return transformationMatrix;
}

//------------------------------------------------------------------------------
Matrix4fR GenerateManualTransformationMatrix(const DynamicTableParameter::ValueType& tableData)
{
  Matrix4fR transformationMatrix;
  transformationMatrix.fill(0.0F);

  for(int64_t rowIndex = 0; rowIndex < 4; rowIndex++)
  {
    std::vector<double> row = tableData[rowIndex];
    for(int64_t colIndex = 0; colIndex < 4; colIndex++)
    {
      transformationMatrix(rowIndex, colIndex) = static_cast<float>(row[colIndex]);
    }
  }
  return transformationMatrix;
}

//------------------------------------------------------------------------------
Matrix4fR GenerateRotationTransformationMatrix(const VectorFloat32Parameter::ValueType& pRotationValue)
{
  Matrix4fR transformationMatrix;
  transformationMatrix.fill(0.0F);

  // Convert Degrees to Radians for the last element
  const float rotAngle = pRotationValue[3] * nx::core::Constants::k_PiOver180F;
  // Ensure the axis part is normalized
  FloatVec3 normalizedAxis = FloatVec3(pRotationValue[0], pRotationValue[1], pRotationValue[2]).normalize();

  const float cosTheta = cos(rotAngle);
  const float oneMinusCosTheta = 1 - cosTheta;
  const float sinTheta = sin(rotAngle);

  // First Row:
  transformationMatrix(0) = normalizedAxis[0] * normalizedAxis[0] * (oneMinusCosTheta) + cosTheta;
  transformationMatrix(1) = normalizedAxis[1] * normalizedAxis[0] * (oneMinusCosTheta) - (normalizedAxis[2] * sinTheta);
  transformationMatrix(2) = normalizedAxis[2] * normalizedAxis[0] * (oneMinusCosTheta) + (normalizedAxis[1] * sinTheta);
  transformationMatrix(3) = 0.0F;

  // Second Row:
  transformationMatrix(4) = normalizedAxis[0] * normalizedAxis[1] * (oneMinusCosTheta) + (normalizedAxis[2] * sinTheta);
  transformationMatrix(5) = normalizedAxis[1] * normalizedAxis[1] * (oneMinusCosTheta) + cosTheta;
  transformationMatrix(6) = normalizedAxis[2] * normalizedAxis[1] * (oneMinusCosTheta) - (normalizedAxis[0] * sinTheta);
  transformationMatrix(7) = 0.0F;

  // Third Row:
  transformationMatrix(8) = normalizedAxis[0] * normalizedAxis[2] * (oneMinusCosTheta) - (normalizedAxis[1] * sinTheta);
  transformationMatrix(9) = normalizedAxis[1] * normalizedAxis[2] * (oneMinusCosTheta) + (normalizedAxis[0] * sinTheta);
  transformationMatrix(10) = normalizedAxis[2] * normalizedAxis[2] * (oneMinusCosTheta) + cosTheta;
  transformationMatrix(11) = 0.0F;

  // Fourth Row:
  transformationMatrix(12) = 0.0F;
  transformationMatrix(13) = 0.0F;
  transformationMatrix(14) = 0.0F;
  transformationMatrix(15) = 1.0F;
  return transformationMatrix;
}

//------------------------------------------------------------------------------
Matrix4fR GenerateTranslationTransformationMatrix(const VectorFloat32Parameter::ValueType& pTranslationValue)
{
  Matrix4fR transformationMatrix = Matrix4fR::Identity();
  transformationMatrix(0, 3) = pTranslationValue[0];
  transformationMatrix(1, 3) = pTranslationValue[1];
  transformationMatrix(2, 3) = pTranslationValue[2];
  return transformationMatrix;
}

//------------------------------------------------------------------------------
Matrix4fR GenerateScaleTransformationMatrix(const VectorFloat32Parameter::ValueType& pScaleValue)
{
  Matrix4fR transformationMatrix;
  transformationMatrix.fill(0.0F);
  transformationMatrix(0, 0) = pScaleValue[0];
  transformationMatrix(1, 1) = pScaleValue[1];
  transformationMatrix(2, 2) = pScaleValue[2];
  transformationMatrix(3, 3) = 1.0f;
  return transformationMatrix;
}

template <class T>
constexpr T square(T value) noexcept
{
  return value * value;
}

/**
 * @brief This function will figure out which octant that the interpolation values should be copied from.
 * @param params
 * @param centerPoint The center point of the target voxel
 * @param coord The coordinate that will be interpolated.
 * @return
 */
size_t FindOctant(const RotateArgs& params, const Point3Df& centerPoint, const Eigen::Array4f& coord)
{
  const float xResHalf = params.xRes * 0.5F;
  const float yResHalf = params.yRes * 0.5F;
  const float zResHalf = params.zRes * 0.5F;

  // Form the 8 corner coords for the voxel
  /* clang-format off */
  std::array<FloatVec3, 8> unitSquareCoords = {
  /* P1 */ FloatVec3(centerPoint[0]-xResHalf, centerPoint[1]-yResHalf, centerPoint[2]-zResHalf),
  /* P2 */ FloatVec3(centerPoint[0]+xResHalf, centerPoint[1]-yResHalf, centerPoint[2]-zResHalf),
  /* P3 */ FloatVec3(centerPoint[0]+xResHalf, centerPoint[1]+yResHalf, centerPoint[2]-zResHalf),
  /* P4 */ FloatVec3(centerPoint[0]-xResHalf, centerPoint[1]+yResHalf, centerPoint[2]-zResHalf),
  /* P5 */ FloatVec3(centerPoint[0]-xResHalf, centerPoint[1]-yResHalf, centerPoint[2]+zResHalf),
  /* P6 */ FloatVec3(centerPoint[0]+xResHalf, centerPoint[1]-yResHalf, centerPoint[2]+zResHalf),
  /* P7 */ FloatVec3(centerPoint[0]+xResHalf, centerPoint[1]+yResHalf, centerPoint[2]+zResHalf),
  /* P8 */ FloatVec3(centerPoint[0]-xResHalf, centerPoint[1]+yResHalf, centerPoint[2]+zResHalf),
  };
  /* clang-format on */

  //  Now figure out which corner the inverse transformed point is closest to
  //  this will give us which octant the point lies.
  float minDistance = std::numeric_limits<float>::max();
  size_t minIndex = 0;
  for(size_t i = 0; i < 8; i++)
  {
    float const distance = square(unitSquareCoords[i][0] - coord[0]) + square(unitSquareCoords[i][1] - coord[1]) + square(unitSquareCoords[i][2] - coord[2]);

    if(distance < minDistance)
    {
      minDistance = distance;
      minIndex = i;
    }
  }

  return minIndex;
}
} // namespace nx::core::ImageRotationUtilities
