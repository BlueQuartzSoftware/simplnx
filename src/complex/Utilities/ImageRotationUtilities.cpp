
#include "ImageRotationUtilities.hpp"

namespace complex::ImageRotationUtilities
{

FloatVec6 DetermineMinMaxCoords(const ImageGeom& imageGeometry, const Matrix4fR& transformationMatrix)
{
  auto origImageGeomBox = imageGeometry.getBoundingBoxf();
  // clang-format off
  std::vector<FloatVec3> imageGeomCornerCoords = {{origImageGeomBox[0], origImageGeomBox[2], origImageGeomBox[4]},
                                                      {origImageGeomBox[1], origImageGeomBox[2], origImageGeomBox[4]},
                                                      {origImageGeomBox[1], origImageGeomBox[3], origImageGeomBox[4]},
                                                      {origImageGeomBox[0], origImageGeomBox[3], origImageGeomBox[4]},
                                                      {origImageGeomBox[0], origImageGeomBox[2], origImageGeomBox[5]},
                                                      {origImageGeomBox[1], origImageGeomBox[2], origImageGeomBox[5]},
                                                      {origImageGeomBox[1], origImageGeomBox[3], origImageGeomBox[5]},
                                                      {origImageGeomBox[0], origImageGeomBox[3], origImageGeomBox[5]}};
  // clang-format on
  FloatVec6 minMaxValues = {std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                            -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max()};

  for(size_t i = 0; i < 8; i++)
  {
    Eigen::Vector4f coords(imageGeomCornerCoords[i][0], imageGeomCornerCoords[i][1], imageGeomCornerCoords[i][2], 1.0F);

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

float CosBetweenVectors(const Eigen::Vector3f& a, const Eigen::Vector3f& b)
{
  const float normA = a.norm();
  const float normB = b.norm();

  if(normA == 0.0f || normB == 0.0f)
  {
    return 1.0f;
  }

  return a.dot(b) / (normA * normB);
}

float DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew)
{
  float xAngle = std::abs(CosBetweenVectors(k_XAxis, axisNew));
  float yAngle = std::abs(CosBetweenVectors(k_YAxis, axisNew));
  float zAngle = std::abs(CosBetweenVectors(k_ZAxis, axisNew));

  std::array<float, 3> axes = {xAngle, yAngle, zAngle};

  auto iter = std::max_element(axes.cbegin(), axes.cend());

  size_t index = std::distance(axes.cbegin(), iter);

  return spacing[index];
}

ImageRotationUtilities::RotateArgs CreateRotationArgs(const ImageGeom& imageGeom, const Matrix4fR& transformationMatrix)
{
  const SizeVec3 origDims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();

  Matrix3fR rotationMatrix = transformationMatrix.block(0, 0, 3, 3);

  FloatVec6 minMaxCoords = DetermineMinMaxCoords(imageGeom, transformationMatrix);

  Eigen::Vector3f xAxisNew = rotationMatrix * k_XAxis;
  Eigen::Vector3f yAxisNew = rotationMatrix * k_YAxis;
  Eigen::Vector3f zAxisNew = rotationMatrix * k_ZAxis;

  float xResNew = DetermineSpacing(spacing, xAxisNew);
  float yResNew = DetermineSpacing(spacing, yAxisNew);
  float zResNew = DetermineSpacing(spacing, zAxisNew);

  IGeometry::MeshIndexType xpNew = static_cast<int64_t>(std::nearbyint((minMaxCoords[1] - minMaxCoords[0]) / xResNew));
  IGeometry::MeshIndexType ypNew = static_cast<int64_t>(std::nearbyint((minMaxCoords[3] - minMaxCoords[2]) / yResNew));
  IGeometry::MeshIndexType zpNew = static_cast<int64_t>(std::nearbyint((minMaxCoords[5] - minMaxCoords[4]) / zResNew));

  ImageRotationUtilities::RotateArgs params;

  params.originalDims = imageGeom.getDimensions();
  params.originalSpacing = imageGeom.getSpacing();
  params.originalOrigin = imageGeom.getOrigin();

  params.xp = origDims[0];
  params.xRes = spacing[0];
  params.yp = origDims[1];
  params.yRes = spacing[1];
  params.zp = origDims[2];
  params.zRes = spacing[2];

  params.transformedSpacing = {xResNew, yResNew, zResNew};
  params.transformedDims = {xpNew, ypNew, zpNew};
  params.transformedOrigin = {minMaxCoords[0], minMaxCoords[2], minMaxCoords[4]};

  params.xpNew = xpNew;
  params.xResNew = xResNew;
  params.xMinNew = minMaxCoords[0];
  params.ypNew = ypNew;
  params.yResNew = yResNew;
  params.yMinNew = minMaxCoords[2];
  params.zpNew = zpNew;
  params.zResNew = zResNew;
  params.zMinNew = minMaxCoords[4];

  return params;
}

Point3D<float32> GetCoords(const USizeVec3& m_Dimensions, const FloatVec3& m_Spacing, const FloatVec3& m_Origin, usize idx)
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float32> coords;
  coords[0] = column * m_Spacing[0] + m_Origin[0] + (0.5f * m_Spacing[0]);
  coords[1] = row * m_Spacing[1] + m_Origin[1] + (0.5f * m_Spacing[1]);
  coords[2] = plane * m_Spacing[2] + m_Origin[2] + (0.5f * m_Spacing[2]);
  return coords;
}

ImageRotationUtilities::Matrix4fR GenerateManualTransformationMatrix(const DynamicTableParameter::ValueType& tableData)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;

  for(size_t rowIndex = 0; rowIndex < 4; rowIndex++)
  {
    std::vector<double> row = tableData[rowIndex];
    for(size_t colIndex = 0; colIndex < 4; colIndex++)
    {
      transformationMatrix(rowIndex, colIndex) = static_cast<float>(row[colIndex]);
    }
  }
  return transformationMatrix;
}

ImageRotationUtilities::Matrix4fR GenerateRotationTransformationMatrix(const VectorFloat32Parameter::ValueType& pRotationValue)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;

  // Convert Degrees to Radians for the last element
  float rotAngle = pRotationValue[3] * Constants::k_PiOver180F;
  // Ensure the axis part is normalized
  FloatVec3 normalizedAxis(pRotationValue[0], pRotationValue[1], pRotationValue[2]);
  MatrixMath::Normalize3x1<float32>(normalizedAxis.data());

  float cosTheta = cos(rotAngle);
  float oneMinusCosTheta = 1 - cosTheta;
  float sinTheta = sin(rotAngle);
  float l = normalizedAxis[0];
  float m = normalizedAxis[1];
  float n = normalizedAxis[2];

  // First Row:
  transformationMatrix(0) = l * l * (oneMinusCosTheta) + cosTheta;
  transformationMatrix(1) = m * l * (oneMinusCosTheta) - (n * sinTheta);
  transformationMatrix(2) = n * l * (oneMinusCosTheta) + (m * sinTheta);
  transformationMatrix(3) = 0.0F;

  // Second Row:
  transformationMatrix(4) = l * m * (oneMinusCosTheta) + (n * sinTheta);
  transformationMatrix(5) = m * m * (oneMinusCosTheta) + cosTheta;
  transformationMatrix(6) = n * m * (oneMinusCosTheta) - (l * sinTheta);
  transformationMatrix(7) = 0.0F;

  // Third Row:
  transformationMatrix(8) = l * n * (oneMinusCosTheta) - (m * sinTheta);
  transformationMatrix(9) = m * n * (oneMinusCosTheta) + (l * sinTheta);
  transformationMatrix(10) = n * n * (oneMinusCosTheta) + cosTheta;
  transformationMatrix(11) = 0.0F;

  // Fourth Row:
  transformationMatrix(12) = 0.0F;
  transformationMatrix(13) = 0.0F;
  transformationMatrix(14) = 0.0F;
  transformationMatrix(15) = 1.0F;
  return transformationMatrix;
}

ImageRotationUtilities::Matrix4fR GenerateTranslationTransformationMatrix(const VectorFloat32Parameter::ValueType& pTranslationValue)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;
  transformationMatrix(0, 0) = 1.0f;
  transformationMatrix(1, 1) = 1.0f;
  transformationMatrix(2, 2) = 1.0f;
  transformationMatrix(3, 3) = 1.0f;
  transformationMatrix(0, 3) = pTranslationValue[0];
  transformationMatrix(1, 3) = pTranslationValue[1];
  transformationMatrix(2, 3) = pTranslationValue[2];
  return transformationMatrix;
}

ImageRotationUtilities::Matrix4fR GenerateScaleTransformationMatrix(const VectorFloat32Parameter::ValueType& pScaleValue)
{
  ImageRotationUtilities::Matrix4fR transformationMatrix;
  transformationMatrix(0, 0) = pScaleValue[0];
  transformationMatrix(1, 1) = pScaleValue[1];
  transformationMatrix(2, 2) = pScaleValue[2];
  transformationMatrix(3, 3) = 1.0f;
  return transformationMatrix;
}

size_t FindOctant(const RotateArgs& params, size_t index, FloatVec3 coord)
{

  float xResHalf = params.xRes * 0.5;
  float yResHalf = params.yRes * 0.5;
  float zResHalf = params.zRes * 0.5;

  // Get the center coord of the original source voxel
  auto centerPt = GetCoords(params.originalDims, params.originalSpacing, params.originalOrigin, index);
  Eigen::Vector3f centerPoint(centerPt[0], centerPt[1], centerPt[2]);

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

  FloatVec3 temp;
  // Now figure out which corner the inverse transformed point is closest to
  // this will give us which octant the point lies.
  float minDistance = std::numeric_limits<float>::max();
  size_t minIndex = 0;
  for(size_t i = 0; i < 8; i++)
  {
    auto tempVec = (unitSquareCoords[i] - coord);
    float const distance = tempVec.dot(tempVec);
    if(distance < minDistance)
    {
      minDistance = distance;
      minIndex = i;
    }
  }

  return minIndex;
}

} // namespace complex::ImageRotationUtilities
