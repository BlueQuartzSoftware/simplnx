#include "ComputeShapes.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

#include <array>
#include <cmath>
#include <utility>
namespace
{
/**
 * @brief Sorts the 3 values
 * @param a First Value
 * @param b Second Value
 * @param c Third Value
 * @return The indices in their sorted order
 */
template <typename T>
std::array<size_t, 3> TripletSort(T a, T b, T c, bool lowToHigh)
{
  constexpr size_t A = 0;
  constexpr size_t B = 1;
  constexpr size_t C = 2;
  std::array<size_t, 3> idx = {0, 1, 2};
  if(a > b && a > c)
  {
    // sorted[2] = a;
    if(b > c)
    {
      // sorted[1] = b;
      // sorted[0] = c;
      idx = {C, B, A};
    }
    else
    {
      // sorted[1] = c;
      // sorted[0] = b;
      idx = {B, C, A};
    }
  }
  else if(b > a && b > c)
  {
    // sorted[2] = b;
    if(a > c)
    {
      // sorted[1] = a;
      // sorted[0] = c;
      idx = {C, A, B};
    }
    else
    {
      // sorted[1] = c;
      // sorted[0] = a;
      idx = {A, C, B};
    }
  }
  else if(a > b)
  {
    // sorted[1] = a;
    // sorted[0] = b;
    // sorted[2] = c;
    idx = {B, A, C};
  }
  else if(a >= c && b >= c)
  {
    // sorted[0] = c;
    // sorted[1] = a;
    // sorted[2] = b;
    idx = {C, A, B};
  }
  else
  {
    // sorted[0] = a;
    // sorted[1] = b;
    // sorted[2] = c;
    idx = {A, B, C};
  }

  if(!lowToHigh)
  {
    std::swap(idx[0], idx[2]);
  }
  return idx;
}

} // namespace
using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeShapes::ComputeShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeShapesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeShapes::~ComputeShapes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeShapes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeShapes::operator()()
{

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  auto spacing = imageGeom.getSpacing();

  m_ScaleFactor = static_cast<double>(1.0f / spacing[0]);
  if(spacing[1] > spacing[0] && spacing[1] > spacing[2])
  {
    m_ScaleFactor = static_cast<double>(1.0f / spacing[1]);
  }
  if(spacing[2] > spacing[0] && spacing[2] > spacing[1])
  {
    m_ScaleFactor = static_cast<double>(1.0f / spacing[2]);
  }

  auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

  size_t numfeatures = centroids.getNumberOfTuples();

  m_FeatureMoments.assign(numfeatures * 6, 0.0F);
  m_FeatureEigenVals.assign(numfeatures * 3, 0.0F);

  // this is a temp array that is used during the calculations
  m_EFVec.resize(numfeatures * 9);

  if(imageGeom.getNumXCells() > 1 && imageGeom.getNumYCells() > 1 && imageGeom.getNumZCells() > 1)
  {
    find_moments();
    find_axes();
    find_axiseulers();
  }
  if(imageGeom.getNumXCells() == 1 || imageGeom.getNumYCells() == 1 || imageGeom.getNumZCells() == 1)
  {
    find_moments2D();
    find_axes2D();
    find_axiseulers2D();
  }

  return {};
}

// -----------------------------------------------------------------------------
void ComputeShapes::find_moments()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  auto& volumes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VolumesArrayPath);
  auto& omega3s = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->Omega3sArrayPath);

  float u200 = 0.0f;
  float u020 = 0.0f;
  float u002 = 0.0f;
  float u110 = 0.0f;
  float u011 = 0.0f;
  float u101 = 0.0f;
  float xx = 0.0f, yy = 0.0f, zz = 0.0f, xy = 0.0f, xz = 0.0f, yz = 0.0f;

  size_t xPoints = imageGeom.getNumXCells();
  size_t yPoints = imageGeom.getNumYCells();
  size_t zPoints = imageGeom.getNumZCells();
  FloatVec3 spacing = imageGeom.getSpacing();
  FloatVec3 origin = imageGeom.getOrigin();

  // using a modified resolution to keep the moment calculations "small" and prevent exceeding numerical bounds.
  // scaleFactor is applied later to rescale the calculated axis lengths
  float modXRes = spacing[0] * static_cast<float>(m_ScaleFactor);
  float modYRes = spacing[1] * static_cast<float>(m_ScaleFactor);
  float modZRes = spacing[2] * static_cast<float>(m_ScaleFactor);

  size_t numfeatures = centroids.getNumberOfTuples();

  float x = 0.0f, y = 0.0f, z = 0.0f, x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f, z1 = 0.0f, z2 = 0.0f;
  float xdist1 = 0.0f, xdist2 = 0.0f, xdist3 = 0.0f, xdist4 = 0.0f, xdist5 = 0.0f, xdist6 = 0.0f, xdist7 = 0.0f, xdist8 = 0.0f;
  float ydist1 = 0.0f, ydist2 = 0.0f, ydist3 = 0.0f, ydist4 = 0.0f, ydist5 = 0.0f, ydist6 = 0.0f, ydist7 = 0.0f, ydist8 = 0.0f;
  float zdist1 = 0.0f, zdist2 = 0.0f, zdist3 = 0.0f, zdist4 = 0.0f, zdist5 = 0.0f, zdist6 = 0.0f, zdist7 = 0.0f, zdist8 = 0.0f;
  size_t zStride = 0, yStride = 0;
  for(size_t i = 0; i < zPoints; i++)
  {
    zStride = i * xPoints * yPoints;
    for(size_t j = 0; j < yPoints; j++)
    {
      yStride = j * xPoints;
      for(size_t k = 0; k < xPoints; k++)
      {
        int32_t gnum = featureIds[zStride + yStride + k];
        x = float(k * modXRes) + (origin[0] * static_cast<float>(m_ScaleFactor));
        y = float(j * modYRes) + (origin[1] * static_cast<float>(m_ScaleFactor));
        z = float(i * modZRes) + (origin[2] * static_cast<float>(m_ScaleFactor));
        x1 = x + (modXRes / 4.0f);
        x2 = x - (modXRes / 4.0f);
        y1 = y + (modYRes / 4.0f);
        y2 = y - (modYRes / 4.0f);
        z1 = z + (modZRes / 4.0f);
        z2 = z - (modZRes / 4.0f);
        xdist1 = (x1 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
        ydist1 = (y1 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
        zdist1 = (z1 - (centroids[gnum * 3 + 2] * static_cast<float>(m_ScaleFactor)));
        xdist2 = (x1 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
        ydist2 = (y1 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
        zdist2 = (z2 - (centroids[gnum * 3 + 2] * static_cast<float>(m_ScaleFactor)));
        xdist3 = (x1 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
        ydist3 = (y2 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
        zdist3 = (z1 - (centroids[gnum * 3 + 2] * static_cast<float>(m_ScaleFactor)));
        xdist4 = (x1 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
        ydist4 = (y2 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
        zdist4 = (z2 - (centroids[gnum * 3 + 2] * static_cast<float>(m_ScaleFactor)));
        xdist5 = (x2 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
        ydist5 = (y1 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
        zdist5 = (z1 - (centroids[gnum * 3 + 2] * static_cast<float>(m_ScaleFactor)));
        xdist6 = (x2 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
        ydist6 = (y1 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
        zdist6 = (z2 - (centroids[gnum * 3 + 2] * static_cast<float>(m_ScaleFactor)));
        xdist7 = (x2 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
        ydist7 = (y2 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
        zdist7 = (z1 - (centroids[gnum * 3 + 2] * static_cast<float>(m_ScaleFactor)));
        xdist8 = (x2 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
        ydist8 = (y2 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
        zdist8 = (z2 - (centroids[gnum * 3 + 2] * static_cast<float>(m_ScaleFactor)));

        xx = ((ydist1) * (ydist1)) + ((zdist1) * (zdist1)) + ((ydist2) * (ydist2)) + ((zdist2) * (zdist2)) + ((ydist3) * (ydist3)) + ((zdist3) * (zdist3)) + ((ydist4) * (ydist4)) +
             ((zdist4) * (zdist4)) + ((ydist5) * (ydist5)) + ((zdist5) * (zdist5)) + ((ydist6) * (ydist6)) + ((zdist6) * (zdist6)) + ((ydist7) * (ydist7)) + ((zdist7) * (zdist7)) +
             ((ydist8) * (ydist8)) + ((zdist8) * (zdist8));
        yy = ((xdist1) * (xdist1)) + ((zdist1) * (zdist1)) + ((xdist2) * (xdist2)) + ((zdist2) * (zdist2)) + ((xdist3) * (xdist3)) + ((zdist3) * (zdist3)) + ((xdist4) * (xdist4)) +
             ((zdist4) * (zdist4)) + ((xdist5) * (xdist5)) + ((zdist5) * (zdist5)) + ((xdist6) * (xdist6)) + ((zdist6) * (zdist6)) + ((xdist7) * (xdist7)) + ((zdist7) * (zdist7)) +
             ((xdist8) * (xdist8)) + ((zdist8) * (zdist8));
        zz = ((xdist1) * (xdist1)) + ((ydist1) * (ydist1)) + ((xdist2) * (xdist2)) + ((ydist2) * (ydist2)) + ((xdist3) * (xdist3)) + ((ydist3) * (ydist3)) + ((xdist4) * (xdist4)) +
             ((ydist4) * (ydist4)) + ((xdist5) * (xdist5)) + ((ydist5) * (ydist5)) + ((xdist6) * (xdist6)) + ((ydist6) * (ydist6)) + ((xdist7) * (xdist7)) + ((ydist7) * (ydist7)) +
             ((xdist8) * (xdist8)) + ((ydist8) * (ydist8));
        xy = ((xdist1) * (ydist1)) + ((xdist2) * (ydist2)) + ((xdist3) * (ydist3)) + ((xdist4) * (ydist4)) + ((xdist5) * (ydist5)) + ((xdist6) * (ydist6)) + ((xdist7) * (ydist7)) +
             ((xdist8) * (ydist8));
        yz = ((ydist1) * (zdist1)) + ((ydist2) * (zdist2)) + ((ydist3) * (zdist3)) + ((ydist4) * (zdist4)) + ((ydist5) * (zdist5)) + ((ydist6) * (zdist6)) + ((ydist7) * (zdist7)) +
             ((ydist8) * (zdist8));
        xz = ((xdist1) * (zdist1)) + ((xdist2) * (zdist2)) + ((xdist3) * (zdist3)) + ((xdist4) * (zdist4)) + ((xdist5) * (zdist5)) + ((xdist6) * (zdist6)) + ((xdist7) * (zdist7)) +
             ((xdist8) * (zdist8));

        m_FeatureMoments[gnum * 6 + 0] = m_FeatureMoments[gnum * 6 + 0] + static_cast<double>(xx);
        m_FeatureMoments[gnum * 6 + 1] = m_FeatureMoments[gnum * 6 + 1] + static_cast<double>(yy);
        m_FeatureMoments[gnum * 6 + 2] = m_FeatureMoments[gnum * 6 + 2] + static_cast<double>(zz);
        m_FeatureMoments[gnum * 6 + 3] = m_FeatureMoments[gnum * 6 + 3] + static_cast<double>(xy);
        m_FeatureMoments[gnum * 6 + 4] = m_FeatureMoments[gnum * 6 + 4] + static_cast<double>(yz);
        m_FeatureMoments[gnum * 6 + 5] = m_FeatureMoments[gnum * 6 + 5] + static_cast<double>(xz);
        volumes[gnum] = volumes[gnum] + 1.0;
      }
    }
  }
  double sphere = (2000.0 * M_PI * M_PI) / 9.0;
  // constant for moments because voxels are broken into smaller voxels
  double konst1 = static_cast<double>((modXRes / 2.0) * (modYRes / 2.0) * (modZRes / 2.0));
  // constant for volumes because voxels are counted as one
  double konst2 = static_cast<double>((spacing[0]) * (spacing[1]) * (spacing[2]));
  double konst3 = static_cast<double>((modXRes) * (modYRes) * (modZRes));
  double o3 = 0.0, vol5 = 0.0, omega3 = 0.0;
  for(size_t featureId = 1; featureId < numfeatures; featureId++)
  {
    // calculating the modified volume for the omega3 value
    vol5 = volumes[featureId] * konst3;
    volumes[featureId] = volumes[featureId] * konst2;
    m_FeatureMoments[featureId * 6 + 0] = m_FeatureMoments[featureId * 6 + 0] * konst1;
    m_FeatureMoments[featureId * 6 + 1] = m_FeatureMoments[featureId * 6 + 1] * konst1;
    m_FeatureMoments[featureId * 6 + 2] = m_FeatureMoments[featureId * 6 + 2] * konst1;
    m_FeatureMoments[featureId * 6 + 3] = -m_FeatureMoments[featureId * 6 + 3] * konst1;
    m_FeatureMoments[featureId * 6 + 4] = -m_FeatureMoments[featureId * 6 + 4] * konst1;
    m_FeatureMoments[featureId * 6 + 5] = -m_FeatureMoments[featureId * 6 + 5] * konst1;

    // Now store the 3x3 Matrix for the Eigen Value/Vectors
    Eigen::Matrix3f moment;
    // clang-format off
    moment <<
      m_FeatureMoments[featureId * 6 + 0], m_FeatureMoments[featureId * 6 + 3], m_FeatureMoments[featureId * 6 + 5],
      m_FeatureMoments[featureId * 6 + 3], m_FeatureMoments[featureId * 6 + 1], m_FeatureMoments[featureId * 6 + 4],
      m_FeatureMoments[featureId * 6 + 5], m_FeatureMoments[featureId * 6 + 4], m_FeatureMoments[featureId * 6 + 2];
    // clang-format on
    Eigen::EigenSolver<Eigen::Matrix3f> es(moment);
    Eigen::EigenSolver<Eigen::Matrix3f>::EigenvalueType eigenValues = es.eigenvalues();
    Eigen::EigenSolver<Eigen::Matrix3f>::EigenvectorsType eigenVectors = es.eigenvectors();

    // Returns the argument order sorted high to low
    std::array<size_t, 3> idxs = ::TripletSort(eigenValues[0].real(), eigenValues[1].real(), eigenValues[2].real(), false);
    m_FeatureEigenVals[featureId * 3 + 0] = eigenValues[idxs[0]].real();
    m_FeatureEigenVals[featureId * 3 + 1] = eigenValues[idxs[1]].real();
    m_FeatureEigenVals[featureId * 3 + 2] = eigenValues[idxs[2]].real();

    // EigenVector associated with the largest EigenValue goes in the 3rd column
    auto col = eigenVectors.col(idxs[0]);
    m_EFVec[featureId * 9 + 2] = col(0).real();
    m_EFVec[featureId * 9 + 5] = col(1).real();
    m_EFVec[featureId * 9 + 8] = col(2).real();

    // Then the next largest into the 2nd column
    col = eigenVectors.col(idxs[1]);
    m_EFVec[featureId * 9 + 1] = col(0).real();
    m_EFVec[featureId * 9 + 4] = col(1).real();
    m_EFVec[featureId * 9 + 7] = col(2).real();

    // The smallest into the 1rst column
    col = eigenVectors.col(idxs[2]);
    m_EFVec[featureId * 9 + 0] = col(0).real();
    m_EFVec[featureId * 9 + 3] = col(1).real();
    m_EFVec[featureId * 9 + 6] = col(2).real();

    // Only for Omega3 below
    u200 = static_cast<float>((m_FeatureMoments[featureId * 6 + 1] + m_FeatureMoments[featureId * 6 + 2] - m_FeatureMoments[featureId * 6 + 0]) / 2.0f);
    u020 = static_cast<float>((m_FeatureMoments[featureId * 6 + 0] + m_FeatureMoments[featureId * 6 + 2] - m_FeatureMoments[featureId * 6 + 1]) / 2.0f);
    u002 = static_cast<float>((m_FeatureMoments[featureId * 6 + 0] + m_FeatureMoments[featureId * 6 + 1] - m_FeatureMoments[featureId * 6 + 2]) / 2.0f);
    u110 = static_cast<float>(-m_FeatureMoments[featureId * 6 + 3]);
    u011 = static_cast<float>(-m_FeatureMoments[featureId * 6 + 4]);
    u101 = static_cast<float>(-m_FeatureMoments[featureId * 6 + 5]);
    o3 = static_cast<double>((u200 * u020 * u002) + (2.0f * u110 * u101 * u011) - (u200 * u011 * u011) - (u020 * u101 * u101) - (u002 * u110 * u110));
    vol5 = pow(vol5, 5.0);
    omega3 = vol5 / o3;
    omega3 = omega3 / sphere;
    if(omega3 > 1)
    {
      omega3 = 1.0;
    }
    if(vol5 == 0.0)
    {
      omega3 = 0.0;
    }
    omega3s[featureId] = static_cast<float>(omega3);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ComputeShapes::find_moments2D()
{

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  auto& volumes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VolumesArrayPath);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  float xx = 0.0f, yy = 0.0f, xy = 0.0f;
  size_t numfeatures = centroids.getNumberOfTuples();

  size_t xPoints = 0, yPoints = 0;
  FloatVec3 spacing = imageGeom.getSpacing();

  if(imageGeom.getNumXCells() == 1)
  {
    xPoints = imageGeom.getNumYCells();
    yPoints = imageGeom.getNumZCells();
    spacing = imageGeom.getSpacing();
  }
  if(imageGeom.getNumYCells() == 1)
  {
    xPoints = imageGeom.getNumXCells();
    yPoints = imageGeom.getNumZCells();
    spacing = imageGeom.getSpacing();
  }
  if(imageGeom.getNumZCells() == 1)
  {
    xPoints = imageGeom.getNumXCells();
    yPoints = imageGeom.getNumYCells();
    spacing = imageGeom.getSpacing();
  }

  float modXRes = spacing[0] * m_ScaleFactor;
  float modYRes = spacing[1] * m_ScaleFactor;

  FloatVec3 origin = imageGeom.getOrigin();

  for(size_t featureId = 0; featureId < 6 * numfeatures; featureId++)
  {
    m_FeatureMoments[featureId] = 0.0;
  }

  size_t yStride = 0;
  for(size_t yPoint = 0; yPoint < yPoints; yPoint++)
  {
    yStride = yPoint * xPoints;
    for(size_t xPoint = 0; xPoint < xPoints; xPoint++)
    {
      int32_t gnum = featureIds[yStride + xPoint];
      float x = static_cast<float>(xPoint * modXRes) + (origin[0] * static_cast<float>(m_ScaleFactor));
      float y = static_cast<float>(yPoint * modYRes) + (origin[1] * static_cast<float>(m_ScaleFactor));
      float x1 = x + (modXRes / 4.0f);
      float x2 = x - (modXRes / 4.0f);
      float y1 = y + (modYRes / 4.0f);
      float y2 = y - (modYRes / 4.0f);
      float xdist1 = (x1 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
      float ydist1 = (y1 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
      float xdist2 = (x1 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
      float ydist2 = (y2 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
      float xdist3 = (x2 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
      float ydist3 = (y1 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
      float xdist4 = (x2 - (centroids[gnum * 3 + 0] * static_cast<float>(m_ScaleFactor)));
      float ydist4 = (y2 - (centroids[gnum * 3 + 1] * static_cast<float>(m_ScaleFactor)));
      xx = ((ydist1) * (ydist1)) + ((ydist2) * (ydist2)) + ((ydist3) * (ydist3)) + ((ydist4) * (ydist4));
      yy = ((xdist1) * (xdist1)) + ((xdist2) * (xdist2)) + ((xdist3) * (xdist3)) + ((xdist4) * (xdist4));
      xy = ((xdist1) * (ydist1)) + ((xdist2) * (ydist2)) + ((xdist3) * (ydist3)) + ((xdist4) * (ydist4));
      m_FeatureMoments[gnum * 6 + 0] = m_FeatureMoments[gnum * 6 + 0] + xx;
      m_FeatureMoments[gnum * 6 + 1] = m_FeatureMoments[gnum * 6 + 1] + yy;
      m_FeatureMoments[gnum * 6 + 2] = m_FeatureMoments[gnum * 6 + 2] + xy;
      volumes[gnum] = volumes[gnum] + 1.0;
    }
  }
  double konst1 = static_cast<double>((modXRes / 2.0f) * (modYRes / 2.0f));
  double konst2 = static_cast<double>(spacing[0] * spacing[1]);
  for(size_t featureId = 1; featureId < numfeatures; featureId++)
  {
    // Eq. 12 Moment matrix. Omega 2
    // Eq. 11 Omega 2
    // E1. 13 Omega 1
    // xx = u20 =
    volumes[featureId] = volumes[featureId] * konst2;                                    // Area
    m_FeatureMoments[featureId * 6 + 0] = m_FeatureMoments[featureId * 6 + 0] * konst1;  // u20
    m_FeatureMoments[featureId * 6 + 1] = m_FeatureMoments[featureId * 6 + 1] * konst1;  // u02
    m_FeatureMoments[featureId * 6 + 2] = -m_FeatureMoments[featureId * 6 + 2] * konst1; // u11
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ComputeShapes::find_axes()
{
  auto& axisLengths = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisLengthsArrayPath);
  auto& aspectRatios = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AspectRatiosArrayPath);

  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

  size_t numfeatures = centroids.getNumberOfTuples();
  constexpr double multiplier = 1.0 / (4.0 * M_PI);
  for(size_t featureId = 1; featureId < numfeatures; featureId++)
  {
    double r1 = m_FeatureEigenVals[3 * featureId];
    double r2 = m_FeatureEigenVals[3 * featureId + 1];
    double r3 = m_FeatureEigenVals[3 * featureId + 2];

    // Adjust to ABC of ellipsoid volume
    double I1 = (15.0 * r1) * multiplier;
    double I2 = (15.0 * r2) * multiplier;
    double I3 = (15.0 * r3) * multiplier;
    double A = (I1 + I2 - I3) * 0.5;
    double B = (I1 + I3 - I2) * 0.5;
    double C = (I2 + I3 - I1) * 0.5;
    double a = (A * A * A * A) / (B * C);
    a = std::pow(a, 0.1);
    double b = B / A;
    b = std::sqrt(b) * a;
    double c = A / (a * a * a * b);

    axisLengths[3 * featureId] = static_cast<float>(a / m_ScaleFactor);
    axisLengths[3 * featureId + 1] = static_cast<float>(b / m_ScaleFactor);
    axisLengths[3 * featureId + 2] = static_cast<float>(c / m_ScaleFactor);
    double bovera = b / a;
    double covera = c / a;
    if(A == 0.0 || B == 0.0 || C == 0.0)
    {
      bovera = 0.0f;
      covera = 0.0f;
    }
    aspectRatios[2 * featureId] = bovera;
    aspectRatios[2 * featureId + 1] = covera;
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ComputeShapes::find_axes2D()
{
  auto& volumes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VolumesArrayPath);
  auto& axisLengths = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisLengthsArrayPath);
  auto& aspectRatios = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AspectRatiosArrayPath);

  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

  double Ixx = 0.0, Iyy = 0.0, Ixy = 0.0;

  size_t numfeatures = centroids.getNumberOfTuples();
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  FloatVec3 spacing;

  if(imageGeom.getNumXCells() == 1)
  {
    spacing = imageGeom.getSpacing();
  }
  if(imageGeom.getNumYCells() == 1)
  {
    spacing = imageGeom.getSpacing();
  }
  if(imageGeom.getNumZCells() == 1)
  {
    spacing = imageGeom.getSpacing();
  }

  double preterm = 4.0 / M_PI;
  preterm = std::pow(preterm, 0.25);

  for(size_t i = 1; i < numfeatures; i++)
  {
    Ixx = m_FeatureMoments[i * 6 + 0];
    Iyy = m_FeatureMoments[i * 6 + 1];
    Ixy = m_FeatureMoments[i * 6 + 2];
    double r1 = (Ixx + Iyy) / 2.0 + std::sqrt(((Ixx + Iyy) * (Ixx + Iyy)) / 4.0 - (Ixx * Iyy - Ixy * Ixy));
    double r2 = (Ixx + Iyy) / 2.0 - std::sqrt(((Ixx + Iyy) * (Ixx + Iyy)) / 4.0 - (Ixx * Iyy - Ixy * Ixy));
    if(r2 <= 0)
    {
      float tempScale1 = 1.0f;
      float tempScale2 = 1.0f;
      if(Ixx >= Iyy)
      {
        tempScale1 = spacing[0];
        tempScale2 = spacing[1];
      }
      if(Ixx < Iyy)
      {
        tempScale1 = spacing[1];
        tempScale2 = spacing[0];
      }
      axisLengths[3 * i] = volumes[i] / tempScale1;
      axisLengths[3 * i + 1] = volumes[i] / tempScale2;
      aspectRatios[2 * i] = tempScale2 / tempScale1;
      aspectRatios[2 * i + 1] = 0.0f;
      continue;
    }

    double postterm1 = r1 * r1 * r1 / r2;
    double postterm2 = r2 * r2 * r2 / r1;
    postterm1 = std::pow(postterm1, 0.125f);
    postterm2 = std::pow(postterm2, 0.125f);
    r1 = preterm * postterm1;
    r2 = preterm * postterm2;
    axisLengths[3 * i] = static_cast<float>(r1 / m_ScaleFactor);
    axisLengths[3 * i + 1] = static_cast<float>(r2 / m_ScaleFactor);
    aspectRatios[2 * i] = static_cast<float>(r2 / r1);
    aspectRatios[2 * i + 1] = 0.0f;
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ComputeShapes::find_axiseulers()
{
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  auto& axisEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisEulerAnglesArrayPath);

  size_t numfeatures = centroids.getNumberOfTuples();
  for(size_t featureId = 1; featureId < numfeatures; featureId++)
  {
    // insert principal unit vectors into rotation matrix representing Feature reference frame within the sample reference frame
    //(Note that the 3 direction is actually the long axis and the 1 direction is actually the short axis)
    // clang-format off
    size_t idx = featureId*9;
    float g[3][3] = {{m_EFVec[idx + 0], m_EFVec[idx + 3], m_EFVec[idx + 6]},
                     {m_EFVec[idx + 1], m_EFVec[idx + 4], m_EFVec[idx + 7]},
                     {m_EFVec[idx + 2], m_EFVec[idx + 5], m_EFVec[idx + 8]}};
    // clang-format on

    // check for right-handedness
    OrientationTransformation::ResultType result = OrientationTransformation::om_check(OrientationF(g));
    if(result.result == 0)
    {
      g[2][0] *= -1.0f;
      g[2][1] *= -1.0f;
      g[2][2] *= -1.0f;
    }

    OrientationF eu = OrientationTransformation::om2eu<OrientationF, OrientationF>(OrientationF(g));

    axisEulerAngles[3 * featureId] = eu[0];
    axisEulerAngles[3 * featureId + 1] = eu[1];
    axisEulerAngles[3 * featureId + 2] = eu[2];
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ComputeShapes::find_axiseulers2D()
{
  const auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);
  auto& axisEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AxisEulerAnglesArrayPath);

  size_t numfeatures = centroids.getNumberOfTuples();

  for(size_t featureId = 1; featureId < numfeatures; featureId++)
  {
    double Ixx = m_FeatureMoments[featureId * 6 + 0];
    double Iyy = m_FeatureMoments[featureId * 6 + 1];
    double Ixy = m_FeatureMoments[featureId * 6 + 2];
    if(Ixy == 0)
    {
      if(Ixx > Iyy)
      {
        axisEulerAngles[3 * featureId] = nx::core::numbers::pi / 180.0F;
        axisEulerAngles[3 * featureId + 1] = 0.0f;
        axisEulerAngles[3 * featureId + 2] = 0.0f;
        continue;
      }
      if(Iyy >= Ixx)
      {
        axisEulerAngles[3 * featureId] = 0.0f;
        axisEulerAngles[3 * featureId + 1] = 0.0f;
        axisEulerAngles[3 * featureId + 2] = 0.0f;
        continue;
      }
    }
    double I1 = (Ixx + Iyy) / 2.0 + std::sqrt(((Ixx + Iyy) * (Ixx + Iyy)) / 4.0 + (Ixy * Ixy - Ixx * Iyy));
    double I2 = (Ixx + Iyy) / 2.0 - std::sqrt(((Ixx + Iyy) * (Ixx + Iyy)) / 4.0 + (Ixy * Ixy - Ixx * Iyy));
    double n1x = (Ixx - I1) / Ixy;
    double n1y = 1.0;
    double n2x = (Ixx - I2) / Ixy;
    double n2y = 1.0;
    double norm1 = sqrt((n1x * n1x + n1y * n1y));
    double norm2 = sqrt((n2x * n2x + n2y * n2y));
    n1x = n1x / norm1;
    n1y = n1y / norm1;
    n2x = n2x / norm2;
    n2y = n2y / norm2;
    double cosine1 = n1x;
    double ea1 = acos(cosine1);
    if(ea1 > nx::core::numbers::pi)
    {
      ea1 = ea1 - nx::core::numbers::pi;
    }
    axisEulerAngles[3 * featureId] = static_cast<float>(ea1);
    axisEulerAngles[3 * featureId + 1] = 0.0f;
    axisEulerAngles[3 * featureId + 2] = 0.0f;
  }
}
