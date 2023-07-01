
#include "ImageRotationUtilities.hpp"

#include <algorithm>

namespace complex::ImageRotationUtilities
{

//------------------------------------------------------------------------------
FloatVec6 DetermineMinMaxCoords(const ImageGeom& imageGeometry, const Matrix4fR& transformationMatrix)
{
  auto origImageGeomBox = imageGeometry.getBoundingBoxf();
  auto min = origImageGeomBox.getMinPoint();
  auto max = origImageGeomBox.getMaxPoint();
  // clang-format off
  std::vector<FloatVec3> imageGeomCornerCoords = {{min[0], min[1], min[2]},
                                                  {min[0], min[1], max[2]},
                                                  {min[0], max[1], min[2]},
                                                  {min[0], max[1], max[2]},
                                                  {max[0], min[1], min[2]},
                                                  {max[0], min[1], max[2]},
                                                  {max[0], max[1], min[2]},
                                                  {max[0], max[1], max[2]}};

  // clang-format on
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
float CosBetweenVectors(const Eigen::Vector3f& vectorA, const Eigen::Vector3f& vectorB)
{
  const float normA = vectorA.norm();
  const float normB = vectorB.norm();

  if(normA == 0.0f || normB == 0.0f)
  {
    return 1.0f;
  }

  return vectorA.dot(vectorB) / (normA * normB);
}

//------------------------------------------------------------------------------
float DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew)
{
  const float xAngle = std::abs(CosBetweenVectors(k_XAxis, axisNew));
  const float yAngle = std::abs(CosBetweenVectors(k_YAxis, axisNew));
  const float zAngle = std::abs(CosBetweenVectors(k_ZAxis, axisNew));

  const std::array<float, 3> axes = {xAngle, yAngle, zAngle};

  const std::array<float, 3>::const_iterator maxElementIter = std::max_element(axes.cbegin(), axes.cend());

  const size_t index = std::distance(axes.cbegin(), maxElementIter);

  return spacing[index];
}

//------------------------------------------------------------------------------
ImageRotationUtilities::RotateArgs CreateRotationArgs(const ImageGeom& imageGeom, const Matrix4fR& transformationMatrix)
{
  const SizeVec3 origDims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();

  const Matrix3fR rotationMatrix = transformationMatrix.block(0, 0, 3, 3);

  FloatVec6 minMaxCoords = DetermineMinMaxCoords(imageGeom, transformationMatrix);

  const Eigen::Vector3f xAxisNew = rotationMatrix * k_XAxis;
  const Eigen::Vector3f yAxisNew = rotationMatrix * k_YAxis;
  const Eigen::Vector3f zAxisNew = rotationMatrix * k_ZAxis;

  const float xResNew = DetermineSpacing(spacing, xAxisNew);
  const float yResNew = DetermineSpacing(spacing, yAxisNew);
  const float zResNew = DetermineSpacing(spacing, zAxisNew);

  const IGeometry::MeshIndexType xpNew = static_cast<int64_t>(std::nearbyint((minMaxCoords[1] - minMaxCoords[0]) / xResNew));
  const IGeometry::MeshIndexType ypNew = static_cast<int64_t>(std::nearbyint((minMaxCoords[3] - minMaxCoords[2]) / yResNew));
  const IGeometry::MeshIndexType zpNew = static_cast<int64_t>(std::nearbyint((minMaxCoords[5] - minMaxCoords[4]) / zResNew));

  ImageRotationUtilities::RotateArgs params;

  params.OriginalDims = imageGeom.getDimensions();
  params.OriginalSpacing = imageGeom.getSpacing();
  params.OriginalOrigin = imageGeom.getOrigin();

  params.xp = static_cast<int64>(origDims[0]);
  params.xRes = spacing[0];
  params.yp = static_cast<int64>(origDims[1]);
  params.yRes = spacing[1];
  params.zp = static_cast<int64>(origDims[2]);
  params.zRes = spacing[2];

  params.TransformedDims = {xpNew, ypNew, zpNew};
  params.TransformedSpacing = {xResNew, yResNew, zResNew};
  params.TransformedOrigin = {minMaxCoords[0], minMaxCoords[2], minMaxCoords[4]};

  params.xpNew = static_cast<int64>(xpNew);
  params.xResNew = xResNew;
  params.xMinNew = minMaxCoords[0];
  params.ypNew = static_cast<int64>(ypNew);
  params.yResNew = yResNew;
  params.yMinNew = minMaxCoords[2];
  params.zpNew = static_cast<int64>(zpNew);
  params.zResNew = zResNew;
  params.zMinNew = minMaxCoords[4];

  return params;
}

//------------------------------------------------------------------------------
std::string GenerateTransformationMatrixDescription(const ImageRotationUtilities::Matrix4fR& transform)
{

  std::stringstream out;

  out << fmt::format("| {:+f}  {:+f}  {:+f}  {:+f} |", transform(0, 0), transform(0, 1), transform(0, 2), transform(0, 3)) << "\n"
      << fmt::format("| {:+f}  {:+f}  {:+f}  {:+f} |", transform(1, 0), transform(1, 1), transform(1, 2), transform(1, 3)) << "\n"
      << fmt::format("| {:+f}  {:+f}  {:+f}  {:+f} |", transform(2, 0), transform(2, 1), transform(2, 2), transform(2, 3)) << "\n"
      << fmt::format("| {:+f}  {:+f}  {:+f}  {:+f} |", transform(3, 0), transform(3, 1), transform(3, 2), transform(3, 3)) << std::endl;

  return out.str();
}

//------------------------------------------------------------------------------
ImageRotationUtilities::Matrix4fR CopyPrecomputedToTransformationMatrix(const Float32Array& precomputed)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;
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
ImageRotationUtilities::Matrix4fR GenerateManualTransformationMatrix(const DynamicTableParameter::ValueType& tableData)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;
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
ImageRotationUtilities::Matrix4fR GenerateRotationTransformationMatrix(const VectorFloat32Parameter::ValueType& pRotationValue)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;
  transformationMatrix.fill(0.0F);

  // Convert Degrees to Radians for the last element
  const float rotAngle = pRotationValue[3] * Constants::k_PiOver180F;
  // Ensure the axis part is normalized
  FloatVec3 normalizedAxis(pRotationValue[0], pRotationValue[1], pRotationValue[2]);
  MatrixMath::Normalize3x1<float32>(normalizedAxis.data());

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
ImageRotationUtilities::Matrix4fR GenerateTranslationTransformationMatrix(const VectorFloat32Parameter::ValueType& pTranslationValue)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;
  transformationMatrix.fill(0.0F);
  transformationMatrix(0, 0) = 1.0f;
  transformationMatrix(1, 1) = 1.0f;
  transformationMatrix(2, 2) = 1.0f;
  transformationMatrix(3, 3) = 1.0f;
  transformationMatrix(0, 3) = pTranslationValue[0];
  transformationMatrix(1, 3) = pTranslationValue[1];
  transformationMatrix(2, 3) = pTranslationValue[2];
  return transformationMatrix;
}

//------------------------------------------------------------------------------
ImageRotationUtilities::Matrix4fR GenerateScaleTransformationMatrix(const VectorFloat32Parameter::ValueType& pScaleValue)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;
  transformationMatrix.fill(0.0F);
  transformationMatrix(0, 0) = pScaleValue[0];
  transformationMatrix(1, 1) = pScaleValue[1];
  transformationMatrix(2, 2) = pScaleValue[2];
  transformationMatrix(3, 3) = 1.0f;
  return transformationMatrix;
}

//------------------------------------------------------------------------------
size_t FindOctant(const RotateArgs& params, const Point3Df& centerPoint, const Eigen::Array4f& coord)
{

  const float xResHalf = params.xRes * 0.5F;
  const float yResHalf = params.yRes * 0.5F;
  const float zResHalf = params.zRes * 0.5F;

  // Form the 8 corner coords for the voxel
  // clang-format off
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
  // clang-format on

  //  Now figure out which corner the inverse transformed point is closest to
  //  this will give us which octant the point lies.
  float minDistance = std::numeric_limits<float>::max();
  size_t minIndex = 0;
  for(size_t i = 0; i < 8; i++)
  {
    float const distance = unitSquareCoords[i][0] * coord[0] + unitSquareCoords[i][1] * coord[1] + unitSquareCoords[i][2] * coord[2];

    if(distance < minDistance)
    {
      minDistance = distance;
      minIndex = i;
    }
  }

  return minIndex;
}

} // namespace complex::ImageRotationUtilities
