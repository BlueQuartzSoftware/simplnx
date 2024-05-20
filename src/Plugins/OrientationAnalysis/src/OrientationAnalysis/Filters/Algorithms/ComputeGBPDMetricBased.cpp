#include "ComputeGBPDMetricBased.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/concurrent_vector.h>
#endif

#include <cmath>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;
namespace fs = std::filesystem;
using LaueOpsShPtrType = std::shared_ptr<LaueOps>;
using LaueOpsContainerType = std::vector<LaueOpsShPtrType>;

namespace gbpd_metric_based
{
/**
 * @brief The TriAreaAndNormals class defines a container that stores the area of a given triangle
 * and the two normals for grains on either side of the triangle
 */
class TriAreaAndNormals
{
public:
  TriAreaAndNormals(float64 triArea, float64 n1x, float64 n1y, float64 n1z, float64 n2x, float64 n2y, float64 n2z)
  : Area(triArea)
  , NormalGrain1X(n1x)
  , NormalGrain1Y(n1y)
  , NormalGrain1Z(n1z)
  , NormalGrain2X(n2x)
  , NormalGrain2Y(n2y)
  , NormalGrain2Z(n2z)
  {
  }
  TriAreaAndNormals() = default;

  float64 Area = 0.0;
  float64 NormalGrain1X = 0.0;
  float64 NormalGrain1Y = 0.0;
  float64 NormalGrain1Z = 0.0;
  float64 NormalGrain2X = 0.0;
  float64 NormalGrain2Y = 0.0;
  float64 NormalGrain2Z = 0.0;

  bool operator<(const TriAreaAndNormals& other) const
  {
    return Area < other.Area;
  }
};

/**
 * @brief The TrianglesSelector class implements a threaded algorithm that determines which triangles to
 * include in the GBPD calculation
 */
class TrianglesSelector
{
public:
  TrianglesSelector(bool excludeTripleLines, const IGeometry::SharedFaceList& triangles, const Int8Array& nodeTypes,
#ifdef SIMPLNX_ENABLE_MULTICORE
                    tbb::concurrent_vector<TriAreaAndNormals>& selectedTriangles,
#else
                    std::vector<TriAreaAndNormals>& selectedTriangles,
#endif
                    int32_t phaseOfInterest, const UInt32Array& crystalStructures, const Float32Array& euler, const Int32Array& phases, const Int32Array& faceLabels, const Float64Array& faceNormals,
                    const Float64Array& faceAreas)
  : m_ExcludeTripleLines(excludeTripleLines)
  , m_Triangles(triangles)
  , m_NodeTypes(nodeTypes)
  , m_SelectedTriangles(selectedTriangles)
  , m_PhaseOfInterest(phaseOfInterest)
  , m_EulerAngles(euler)
  , m_Phases(phases)
  , m_FaceLabels(faceLabels)
  , m_FaceNormals(faceNormals)
  , m_FaceAreas(faceAreas)
  {
    m_OrientationOps = LaueOps::GetAllOrientationOps();
    m_Crystal = crystalStructures[phaseOfInterest];
    m_NSym = m_OrientationOps[m_Crystal]->getNumSymOps();
  }

  virtual ~TrianglesSelector() = default;
  TrianglesSelector(const TrianglesSelector&) = default;
  TrianglesSelector(TrianglesSelector&&) noexcept = delete;
  TrianglesSelector& operator=(const TrianglesSelector&) = delete;
  TrianglesSelector& operator=(TrianglesSelector&&) noexcept = delete;

  void select(usize start, usize end) const
  {
    Eigen::Vector3d g1ea = {0.0, 0.0, 0.0};
    Eigen::Vector3d g2ea = {0.0, 0.0, 0.0};
    Eigen::Vector3d normalLab = {0.0, 0.0, 0.0};
    Eigen::Vector3d normalGrain1 = {0.0, 0.0, 0.0};
    Eigen::Vector3d normalGrain2 = {0.0, 0.0, 0.0};

    for(usize triIdx = start; triIdx < end; triIdx++)
    {
      const int32 feature1 = m_FaceLabels[2 * triIdx];
      const int32 feature2 = m_FaceLabels[2 * triIdx + 1];

      if(feature1 < 1 || feature2 < 1)
      {
        continue;
      }
      if(m_Phases[feature1] != m_Phases[feature2])
      {
        continue;
      }
      if(m_Phases[feature1] != m_PhaseOfInterest || m_Phases[feature2] != m_PhaseOfInterest)
      {
        continue;
      }

      if(m_ExcludeTripleLines)
      {
        if(m_NodeTypes[m_Triangles[triIdx * 3]] != 2 || m_NodeTypes[m_Triangles[triIdx * 3 + 1]] != 2 || m_NodeTypes[m_Triangles[triIdx * 3 + 2]] != 2)
        {
          continue;
        }
      }

      normalLab[0] = static_cast<float64>(m_FaceNormals[3 * triIdx]);
      normalLab[1] = static_cast<float64>(m_FaceNormals[3 * triIdx + 1]);
      normalLab[2] = static_cast<float64>(m_FaceNormals[3 * triIdx + 2]);

      for(int32 whichEa = 0; whichEa < 3; whichEa++)
      {
        g1ea[whichEa] = m_EulerAngles[3 * feature1 + whichEa];
        g2ea[whichEa] = m_EulerAngles[3 * feature2 + whichEa];
      }

      auto oMatrix1 = OrientationTransformation::eu2om<OrientationD, OrientationD>(OrientationD(g1ea[0], g1ea[1], g1ea[2], 3.0));
      auto oMatrix2 = OrientationTransformation::eu2om<OrientationD, OrientationD>(OrientationD(g2ea[0], g2ea[1], g2ea[2], 3.0));

      normalGrain1 = OrientationMatrixToGMatrix(oMatrix1) * normalLab;
      normalGrain2 = OrientationMatrixToGMatrix(oMatrix2) * normalLab;

      m_SelectedTriangles.push_back(TriAreaAndNormals(m_FaceAreas[triIdx], normalGrain1[0], normalGrain1[1], normalGrain1[2], -normalGrain2[0], -normalGrain2[1], -normalGrain2[2]));
    }
  }

  void operator()(const Range& range) const
  {
    select(range.min(), range.max());
  }

private:
  // corresponding to Phase of Interest
  bool m_ExcludeTripleLines;
  const IGeometry::SharedFaceList& m_Triangles;
  const Int8Array& m_NodeTypes;
#ifdef SIMPLNX_ENABLE_MULTICORE
  tbb::concurrent_vector<TriAreaAndNormals>& m_SelectedTriangles;
#else
  std::vector<TriAreaAndNormals>& m_SelectedTriangles;
#endif
  int32 m_PhaseOfInterest;
  LaueOpsContainerType m_OrientationOps;
  uint32 m_Crystal;
  int32 m_NSym;
  const Float32Array& m_EulerAngles;
  const Int32Array& m_Phases;
  const Int32Array& m_FaceLabels;
  const Float64Array& m_FaceNormals;
  const Float64Array& m_FaceAreas;
};

/**
 * @brief The ProbeDistribution class implements a threaded algorithm that determines the distribution values
 * for the GBPD
 */
class ProbeDistribution
{
public:
  ProbeDistribution(std::vector<float64>& distributionValues, std::vector<float64>& errorValues, const std::vector<float64>& samplePtsX, const std::vector<float64>& samplePtsY,
                    const std::vector<float64>& samplePtsZ,
#ifdef SIMPLNX_ENABLE_MULTICORE
                    const tbb::concurrent_vector<TriAreaAndNormals>& selectedTriangles,
#else
                    const std::vector<TriAreaAndNormals>& selectedTriangles,
#endif
                    float64 limitDist, float64 totalFaceArea, int32 numDistinctGBs, float64 ballVolume, int32 crystal)
  : m_DistributionValues(distributionValues)
  , m_ErrorValues(errorValues)
  , m_SamplePtsX(samplePtsX)
  , m_SamplePtsY(samplePtsY)
  , m_SamplePtsZ(samplePtsZ)
  , m_SelectedTriangles(selectedTriangles)
  , m_LimitDist(limitDist)
  , m_TotalFaceArea(totalFaceArea)
  , m_NumDistinctGBs(numDistinctGBs)
  , m_BallVolume(ballVolume)
  , m_Crystal(crystal)
  {
    m_OrientationOps = LaueOps::GetAllOrientationOps();
    m_NSym = m_OrientationOps[crystal]->getNumSymOps();
  }

  virtual ~ProbeDistribution() = default;
  ProbeDistribution(const ProbeDistribution&) = default;
  ProbeDistribution(ProbeDistribution&&) noexcept = delete;
  ProbeDistribution& operator=(const ProbeDistribution&) = delete;
  ProbeDistribution& operator=(ProbeDistribution&&) noexcept = delete;

  void probe(usize start, usize end) const
  {
    for(usize ptIdx = start; ptIdx < end; ptIdx++)
    {
      float64 c = 0.0;
      const float64 probeNormal[3] = {m_SamplePtsX[ptIdx], m_SamplePtsY[ptIdx], m_SamplePtsZ[ptIdx]};

      for(const auto& selectedTriangle : m_SelectedTriangles)
      {
        const Eigen::Vector3d normal1 = {selectedTriangle.NormalGrain1X, selectedTriangle.NormalGrain1Y, selectedTriangle.NormalGrain1Z};
        const Eigen::Vector3d normal2 = {selectedTriangle.NormalGrain2X, selectedTriangle.NormalGrain2Y, selectedTriangle.NormalGrain2Z};

        for(int32 j = 0; j < m_NSym; j++)
        {
          const Matrix3dR sym = EbsdLibMatrixToEigenMatrix(m_OrientationOps[m_Crystal]->getMatSymOpD(j));

          Eigen::Vector3d symNormal1 = sym * normal1;
          Eigen::Vector3d symNormal2 = sym * normal2;

          for(int32 inversion = 0; inversion <= 1; inversion++)
          {
            float64 sign = 1.0f;
            if(inversion == 1)
            {
              sign = -1.0f;
            }

            const float64 gamma1 = std::acos(sign * (probeNormal[0] * symNormal1[0] + probeNormal[1] * symNormal1[1] + probeNormal[2] * symNormal1[2]));
            const float64 gamma2 = std::acos(sign * (probeNormal[0] * symNormal2[0] + probeNormal[1] * symNormal2[1] + probeNormal[2] * symNormal2[2]));

            if(gamma1 < m_LimitDist)
            {
              // Kahan summation algorithm
              const float64 y = selectedTriangle.Area - c;
              const float64 t = m_DistributionValues[ptIdx] + y;
              c = (t - m_DistributionValues[ptIdx]) - y;
              m_DistributionValues[ptIdx] = t;
            }
            if(gamma2 < m_LimitDist)
            {
              const float64 y = selectedTriangle.Area - c;
              const float64 t = m_DistributionValues[ptIdx] + y;
              c = (t - m_DistributionValues[ptIdx]) - y;
              m_DistributionValues[ptIdx] = t;
            }
          }
        }
      }
      m_ErrorValues[ptIdx] = sqrt(m_DistributionValues[ptIdx] / m_TotalFaceArea / static_cast<float64>(m_NumDistinctGBs)) / m_BallVolume;
      m_DistributionValues[ptIdx] /= m_TotalFaceArea;
      m_DistributionValues[ptIdx] /= m_BallVolume;
    }
  }

  void operator()(const Range& range) const
  {
    probe(range.min(), range.max());
  }

private:
  std::vector<float64>& m_DistributionValues;
  std::vector<float64>& m_ErrorValues;
  std::vector<float64> m_SamplePtsX;
  std::vector<float64> m_SamplePtsY;
  std::vector<float64> m_SamplePtsZ;
#ifdef SIMPLNX_ENABLE_MULTICORE
  const tbb::concurrent_vector<TriAreaAndNormals>& m_SelectedTriangles;
#else
  const std::vector<TriAreaAndNormals>& m_SelectedTriangles;
#endif
  float64 m_LimitDist;
  float64 m_TotalFaceArea;
  int32 m_NumDistinctGBs;
  float64 m_BallVolume;
  LaueOpsContainerType m_OrientationOps;
  uint32 m_Crystal;
  int32 m_NSym;
};

} // namespace gbpd_metric_based

// -----------------------------------------------------------------------------
ComputeGBPDMetricBased::ComputeGBPDMetricBased(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeGBPDMetricBasedInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeGBPDMetricBased::~ComputeGBPDMetricBased() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeGBPDMetricBased::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeGBPDMetricBased::operator()()
{
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureEulerAnglesArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  auto& faceNormals = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SurfaceMeshFaceNormalsArrayPath);
  auto& faceAreas = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SurfaceMeshFaceAreasArrayPath);
  auto& featureFaceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFeatureFaceLabelsArrayPath);
  auto& nodeTypes = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->NodeTypesArrayPath);

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  const IGeometry::SharedFaceList& triangles = triangleGeom.getFacesRef();

  const float64 limitDist = m_InputValues->LimitDist * Constants::k_PiOver180D;

  if(crystalStructures[m_InputValues->PhaseOfInterest] > 10)
  {
    return MakeErrorResult(-8325, "Unsupported CrystalStructure");
  }

  // -------------------- check if directories are ok and if output files can be opened -----------
  // Make sure the file name ends with _1 so the GMT scripts work correctly
  fs::path distributionOutput = m_InputValues->DistOutputFile;
  std::string distFName = m_InputValues->DistOutputFile.stem().string();
  if(!distFName.empty() && !StringUtilities::ends_with(distFName, "_1"))
  {
    distFName = distFName + "_1.dat";
    distributionOutput = fs::path(m_InputValues->DistOutputFile.parent_path() / distFName);
  }

  fs::path errorOutput = m_InputValues->ErrOutputFile;
  std::string errFName = m_InputValues->ErrOutputFile.stem().string();
  if(!errFName.empty() && !StringUtilities::ends_with(errFName, "_1"))
  {
    errFName = errFName + "_1.dat";
    errorOutput = fs::path(m_InputValues->ErrOutputFile.parent_path() / errFName);
  }

  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = CreateOutputDirectories(distributionOutput.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }
  createDirectoriesResult = CreateOutputDirectories(errorOutput.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  std::ofstream distributionOutStream(distributionOutput, std::ios_base::out);
  if(!distributionOutStream.is_open())
  {
    return MakeErrorResult(-8326, fmt::format("Error creating GBPD distribution output file {}", distributionOutput.string()));
  }

  std::ofstream errorOutStream(errorOutput, std::ios_base::out);
  if(!errorOutStream.is_open())
  {
    return MakeErrorResult(-8327, fmt::format("Error creating GBPD distribution errors output file {}", errorOutput.string()));
  }

  // ------------------- before computing the distribution, we must find normalization factors -----
  std::vector<LaueOps::Pointer> mOrientationOps = LaueOps::GetAllOrientationOps();
  auto crystal = static_cast<int32>(crystalStructures[m_InputValues->PhaseOfInterest]);
  const int32 nSym = mOrientationOps[crystal]->getNumSymOps();
  auto ballVolume = static_cast<float64>(nSym) * 2.0 * (1.0 - std::cos(limitDist));

  // ------------------------------ generation of sampling points ----------------------------------
  m_MessageHandler(IFilter::Message::Type::Info, "Generating sampling points");

  // generate "Golden Section Spiral", see http://www.softimageblog.com/archives/115
  const int numSamplePtsWholeSphere = 2 * m_InputValues->NumSamplPts; // here we generate points on the whole sphere
  std::vector<float64> samplePtsXHemisphere(0);
  std::vector<float64> samplePtsYHemisphere(0);
  std::vector<float64> samplePtsZHemisphere(0);
  std::vector<float64> samplePtsX(0);
  std::vector<float64> samplePtsY(0);
  std::vector<float64> samplePtsZ(0);

  const float64 off = 2.0 / static_cast<float64>(numSamplePtsWholeSphere);
  const float64 kConst1 = Constants::k_PiD * (3.0 - std::sqrt(5.0));
  for(int32 ptIdxWholeSphere = 0; ptIdxWholeSphere < numSamplePtsWholeSphere; ptIdxWholeSphere++)
  {
    if(getCancel())
    {
      return {};
    }

    const float64 y = (static_cast<float64>(ptIdxWholeSphere) * off) - 1.0 + (0.5 * off);
    const float64 r = std::sqrt(std::fmax(1.0 - y * y, 0.0));
    const float64 phi = static_cast<float64>(ptIdxWholeSphere) * kConst1;

    const double z = std::sin(phi) * r;

    if(z >= 0.0)
    {
      samplePtsXHemisphere.push_back(std::cos(phi) * r);
      samplePtsYHemisphere.push_back(y);
      samplePtsZHemisphere.push_back(z);
    }
  }

  // now, select the points from the SST
  for(usize ptIdxHemisphere = 0; ptIdxHemisphere < samplePtsXHemisphere.size(); ptIdxHemisphere++)
  {
    if(getCancel())
    {
      return {};
    }

    const float64 x = samplePtsXHemisphere[ptIdxHemisphere];
    const float64 y = samplePtsYHemisphere[ptIdxHemisphere];
    const float64 z = samplePtsZHemisphere[ptIdxHemisphere];

    if(crystal == 0) // 6/mmm
    {
      if(x < 0.0 || y < 0.0 || y > x * Constants::k_1OverRoot3D)
      {
        continue;
      }
    }
    if(crystal == 1) // m-3m
    {
      if(y < 0.0 || x < y || z < x)
      {
        continue;
      }
    }
    if(crystal == 2 || crystal == 10) // 6/m || -3m
    {
      if(x < 0.0 || y < 0.0 || y > x * Constants::k_Sqrt3D)
      {
        continue;
      }
    }
    if(crystal == 3) // m-3
    {
      if(x < 0.0 || y < 0.0 || z < x || z < y)
      {
        continue;
      }
    }
    // m_Crystal = 4  =>  -1
    if(crystal == 5) // 2/m
    {
      if(y < 0.0)
      {
        continue;
      }
    }
    if(crystal == 6 || crystal == 7) // mmm || 4/m
    {
      if(x < 0.0 || y < 0.0)
      {
        continue;
      }
    }
    if(crystal == 8) // 4/mmm
    {
      if(x < 0.0 || y < 0.0 || y > x)
      {
        continue;
      }
    }
    if(crystal == 9) // -3
    {
      if(y < 0.0 || x < -y * Constants::k_1OverRoot3D)
      {
        continue;
      }
    }

    samplePtsX.push_back(x);
    samplePtsY.push_back(y);
    samplePtsZ.push_back(z);
  }

  // Add points at the edges and vertices of a fundamental region
  if(crystal == 0) // 6/mmm
  {
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 0.0, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 30.0 * Constants::k_PiOver180D, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedZenith(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 30.0 * Constants::k_PiOver180D, limitDist);
  }
  if(crystal == 1) // m-3m
  {
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 0.0, 0.0, 45.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 45.0 * Constants::k_PiOver180D, 0.0, std::acos(Constants::k_1OverRoot3D), limitDist);

    for(float64 phi = 0; phi <= 45.0 * Constants::k_PiOver180D; phi += limitDist)
    {
      const float64 cosPhi = std::cos(phi);
      const float64 sinPhi = std::sin(phi);
      const float64 atan1OverCosPhi = std::atan(1.0 / cosPhi);
      const float64 sinAtan1OverCosPhi = std::sin(atan1OverCosPhi);
      float64 cosAtan1OverCosPhi = std::cos(atan1OverCosPhi);

      samplePtsX.push_back(static_cast<float64>(sinAtan1OverCosPhi * cosPhi));
      samplePtsY.push_back(static_cast<float64>(sinAtan1OverCosPhi * sinPhi));
      samplePtsZ.push_back(static_cast<float64>(cosAtan1OverCosPhi));
    }
  }
  if(crystal == 2 || crystal == 10) // 6/m ||  -3m
  {
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 0.0, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 60.0 * Constants::k_PiOver180D, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedZenith(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 60.0 * Constants::k_PiOver180D, limitDist);
  }
  if(crystal == 3) // m-3
  {
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 0.0, 0.0, 45.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 45.0 * Constants::k_PiOver180D, limitDist);
    for(float64 phi = 0; phi <= 45.0 * Constants::k_PiOver180D; phi += limitDist)
    {
      const float64 cosPhi = std::cos(phi);
      const float64 sinPhi = std::sin(phi);
      const float64 atan1OverCosPhi = std::atan(1.0 / cosPhi);
      const float64 sinAtan1OverCosPhi = std::sin(atan1OverCosPhi);
      float64 cosAtan1OverCosPhi = std::cos(atan1OverCosPhi);

      samplePtsX.push_back(static_cast<float64>(sinAtan1OverCosPhi * cosPhi));
      samplePtsY.push_back(static_cast<float64>(sinAtan1OverCosPhi * sinPhi));
      samplePtsZ.push_back(static_cast<float64>(cosAtan1OverCosPhi));

      samplePtsX.push_back(static_cast<float64>(sinAtan1OverCosPhi * sinPhi));
      samplePtsY.push_back(static_cast<float64>(sinAtan1OverCosPhi * cosPhi));
      samplePtsZ.push_back(static_cast<float64>(cosAtan1OverCosPhi));
    }
  }
  if(crystal == 4) // -1
  {
    AppendSamplePtsFixedZenith(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 360.0 * Constants::k_PiOver180D, limitDist);
  }
  if(crystal == 5) // 2/m
  {
    AppendSamplePtsFixedZenith(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 180.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 0.0, -90.0 * Constants::k_PiOver180D, 90.0 * Constants::k_PiOver180D, limitDist);
  }
  if(crystal == 6 || crystal == 7) // mmm || 4/m
  {
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 0.0, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedZenith(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
  }
  if(crystal == 8) // 4/mmm
  {
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 0.0, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 45.0 * Constants::k_PiOver180D, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedZenith(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 45.0 * Constants::k_PiOver180D, limitDist);
  }
  if(crystal == 9) // -3
  {
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 0.0, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedAzimuth(samplePtsX, samplePtsY, samplePtsZ, 120.0 * Constants::k_PiOver180D, 0.0, 90.0 * Constants::k_PiOver180D, limitDist);
    AppendSamplePtsFixedZenith(samplePtsX, samplePtsY, samplePtsZ, 90.0 * Constants::k_PiOver180D, 0.0, 120.0 * Constants::k_PiOver180D, limitDist);
  }

  // ---------  find triangles corresponding to Phase of Interests, and their normals in crystal reference frames ---------
  const usize numMeshTriangles = faceAreas.getNumberOfTuples();

#ifdef SIMPLNX_ENABLE_MULTICORE
  tbb::concurrent_vector<gbpd_metric_based::TriAreaAndNormals> selectedTriangles(0);
#else
  std::vector<gbpd_metric_based::TriAreaAndNormals> selectedTriangles(0);
#endif

  usize triChunkSize = 50000;
  if(numMeshTriangles < triChunkSize)
  {
    triChunkSize = numMeshTriangles;
  }

  for(usize i = 0; i < numMeshTriangles; i += triChunkSize)
  {
    if(getCancel())
    {
      return {};
    }
    m_MessageHandler(IFilter::Message::Type::Info, "Selecting triangles corresponding to Phase Of Interest");
    if(i + triChunkSize >= numMeshTriangles)
    {
      triChunkSize = numMeshTriangles - i;
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(i, i + triChunkSize);
    dataAlg.execute(gbpd_metric_based::TrianglesSelector(m_InputValues->ExcludeTripleLines, triangles, nodeTypes, selectedTriangles, m_InputValues->PhaseOfInterest, crystalStructures, eulerAngles,
                                                         phases, faceLabels, faceNormals, faceAreas));
  }

  // ------------------------  find the number of distinct boundaries ------------------------------
  int32 numDistinctGBs = 0;
  const usize numFaceFeatures = featureFaceLabels.getNumberOfTuples();

  for(usize featureFaceIdx = 0; featureFaceIdx < numFaceFeatures; featureFaceIdx++)
  {
    const int32 feature1 = featureFaceLabels[2 * featureFaceIdx];
    const int32 feature2 = featureFaceLabels[2 * featureFaceIdx + 1];

    if(feature1 < 1 || feature2 < 1)
    {
      continue;
    }
    if(phases[feature1] != phases[feature2])
    {
      continue;
    }
    if(phases[feature1] != m_InputValues->PhaseOfInterest || phases[feature2] != m_InputValues->PhaseOfInterest)
    {
      continue;
    }

    numDistinctGBs++;
  }

  // ----------------- determining distribution values at the sampling points (and their errors) ---
  float64 totalFaceArea = 0.0;
  for(const auto& selectedTri : selectedTriangles)
  {
    totalFaceArea += selectedTri.Area;
  }

  std::vector<float64> distributionValues(samplePtsX.size(), 0.0);
  std::vector<float64> errorValues(samplePtsX.size(), 0.0);

  usize pointsChunkSize = 20;
  if(samplePtsX.size() < pointsChunkSize)
  {
    pointsChunkSize = samplePtsX.size();
  }

  for(usize i = 0; i < samplePtsX.size(); i = i + pointsChunkSize)
  {
    if(getCancel())
    {
      return {};
    }
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Determining GBPD values ({}%)", static_cast<int32>(100.0 * static_cast<float64>(i) / static_cast<float64>(samplePtsX.size()))));
    if(i + pointsChunkSize >= samplePtsX.size())
    {
      pointsChunkSize = samplePtsX.size() - i;
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(i, i + pointsChunkSize);
    dataAlg.execute(
        gbpd_metric_based::ProbeDistribution(distributionValues, errorValues, samplePtsX, samplePtsY, samplePtsZ, selectedTriangles, limitDist, totalFaceArea, numDistinctGBs, ballVolume, crystal));
  }

  // ------------------------------------------- writing the output --------------------------------
  std::string outputString = fmt::format("{:.1f} {:.1f} {:.1f} {:.1f}\n", 0.0, 0.0, 0.0, 0.0);
  distributionOutStream << outputString;
  errorOutStream << outputString;

  for(usize ptIdx = 0; ptIdx < samplePtsX.size(); ptIdx++)
  {
    const Eigen::Vector3d point = {samplePtsX.at(ptIdx), samplePtsY.at(ptIdx), samplePtsZ.at(ptIdx)};
    Matrix3dR sym;
    sym.fill(0.0);

    for(int32 j = 0; j < nSym; j++)
    {
      sym = EbsdLibMatrixToEigenMatrix(mOrientationOps[crystal]->getMatSymOpD(j));
      Eigen::Vector3d symPoint = {0.0, 0.0, 0.0};
      symPoint = sym * point;

      if(symPoint[2] < 0.0)
      {
        symPoint[0] = -symPoint[0];
        symPoint[1] = -symPoint[1];
        symPoint[2] = -symPoint[2];
      }

      const float64 zenith = std::acos(symPoint[2]);
      const float64 azimuth = std::atan2(symPoint[1], symPoint[0]);

      const float64 zenithDeg = Constants::k_180OverPiD * zenith;
      const float64 azimuthDeg = Constants::k_180OverPiD * azimuth;

      outputString = fmt::format("{:.8E} {:.8E} {:.8E}\n", azimuthDeg, 90.0 - zenithDeg, distributionValues[ptIdx]);
      distributionOutStream << outputString;

      if(!m_InputValues->SaveRelativeErr)
      {
        outputString = fmt::format("{:.8E} {:.8E} {:.8E}\n", azimuthDeg, 90.0 - zenithDeg, errorValues[ptIdx]);
        errorOutStream << outputString;
      }
      else
      {
        float64 saneErr = 100.0;
        if(distributionValues[ptIdx] > 1e-10)
        {
          saneErr = std::fmin(100.0, 100.0 * errorValues[ptIdx] / distributionValues[ptIdx]);
        }
        outputString = fmt::format("{:.8E} {:.8E} {:.8E}\n", azimuthDeg, 90.0 - zenithDeg, saneErr);
        errorOutStream << outputString;
      }
    }
  }
  distributionOutStream.close();
  errorOutStream.close();

  return {};
}

// -----------------------------------------------------------------------------
void ComputeGBPDMetricBased::AppendSamplePtsFixedZenith(std::vector<float64>& xVec, std::vector<float64>& yVec, std::vector<float64>& zVec, float64 theta, float64 minPhi, float64 maxPhi, float64 step)
{
  for(float64 phi = minPhi; phi <= maxPhi; phi += step)
  {
    xVec.push_back(std::sin(theta) * std::cos(phi));
    yVec.push_back(std::sin(theta) * std::sin(phi));
    zVec.push_back(std::cos(theta));
  }
  xVec.push_back(std::sin(theta) * std::cos(maxPhi));
  yVec.push_back(std::sin(theta) * std::sin(maxPhi));
  zVec.push_back(std::cos(theta));
}

// -----------------------------------------------------------------------------
void ComputeGBPDMetricBased::AppendSamplePtsFixedAzimuth(std::vector<float64>& xVec, std::vector<float64>& yVec, std::vector<float64>& zVec, float64 phi, float64 minTheta, float64 maxTheta,
                                                         float64 step)
{
  for(double theta = minTheta; theta <= maxTheta; theta += step)
  {
    xVec.push_back(std::sin(theta) * std::cos(phi));
    yVec.push_back(std::sin(theta) * std::sin(phi));
    zVec.push_back(std::cos(theta));
  }
  xVec.push_back(std::sin(maxTheta) * std::cos(phi));
  yVec.push_back(std::sin(maxTheta) * std::sin(phi));
  zVec.push_back(std::cos(maxTheta));
}
