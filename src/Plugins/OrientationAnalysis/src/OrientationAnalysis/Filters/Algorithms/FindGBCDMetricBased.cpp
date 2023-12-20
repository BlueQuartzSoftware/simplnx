#include "FindGBCDMetricBased.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

#include <Eigen/Dense>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/concurrent_vector.h>
#endif

using namespace nx::core;
using namespace nx::core::OrientationUtilities;
namespace fs = std::filesystem;
using LaueOpsShPtrType = std::shared_ptr<LaueOps>;
using LaueOpsContainerType = std::vector<LaueOpsShPtrType>;

namespace
{
constexpr float64 k_BallVolumesM3M[FindGBCDMetricBased::k_NumberResolutionChoices] = {0.0000641361, 0.000139158, 0.000287439, 0.00038019, 0.000484151, 0.000747069, 0.00145491};
}

namespace GBCDMetricBased
{
/**
 * @brief The TriAreaAndNormals struct defines a container that stores the area of a given triangle
 * and the two normals for grains on either side of the triangle
 */
struct TriAreaAndNormals
{
  TriAreaAndNormals(float64 triArea, float64 n1x, float64 n1y, float64 n1z, float64 n2x, float64 n2y, float64 n2z)
  : Area(triArea)
  , normalGrain1X(n1x)
  , normalGrain1Y(n1y)
  , normalGrain1Z(n1z)
  , normalGrain2X(n2x)
  , normalGrain2Y(n2y)
  , normalGrain2Z(n2z)
  {
  }
  TriAreaAndNormals() = default;

  float64 Area = 0.0;
  float64 normalGrain1X = 0.0;
  float64 normalGrain1Y = 0.0;
  float64 normalGrain1Z = 0.0;
  float64 normalGrain2X = 0.0;
  float64 normalGrain2Y = 0.0;
  float64 normalGrain2Z = 0.0;
};

/**
 * @brief The TrianglesSelector class implements a threaded algorithm that determines which triangles to
 * include in the GBCD calculation
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
                    std::vector<int8>& triIncluded, float64 misResolution, int32 phaseOfInterest, const Matrix3dR& gFixedT, const UInt32Array& crystalStructures, const Float32Array& euler,
                    const Int32Array& phases, const Int32Array& faceLabels, const Float64Array& faceNormals, const Float64Array& faceAreas)
  : m_ExcludeTripleLines(excludeTripleLines)
  , m_Triangles(triangles)
  , m_NodeTypes(nodeTypes)
  , m_SelectedTriangles(selectedTriangles)
  , m_TriIncluded(triIncluded)
  , m_MisResolution(misResolution)
  , m_PhaseOfInterest(phaseOfInterest)
  , m_GFixedT(gFixedT)
  , m_Euler(euler)
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
    Matrix3dR g1s;
    g1s.fill(0.0);
    Matrix3dR g2s;
    g2s.fill(0.0);
    Matrix3dR dg;
    dg.fill(0.0);
    Matrix3dR dgT;
    dgT.fill(0.0);
    Matrix3dR diffFromFixed;
    diffFromFixed.fill(0.0);

    Eigen::Vector3d normalLab = {0.0, 0.0, 0.0};
    Eigen::Vector3d normalGrain1 = {0.0, 0.0, 0.0};
    Eigen::Vector3d normalGrain2 = {0.0, 0.0, 0.0};

    std::set<usize> uniqueTri;

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

      m_TriIncluded[triIdx] = 1;

      normalLab[0] = (m_FaceNormals[3 * triIdx]);
      normalLab[1] = (m_FaceNormals[3 * triIdx + 1]);
      normalLab[2] = (m_FaceNormals[3 * triIdx + 2]);

      for(int whichEa = 0; whichEa < 3; whichEa++)
      {
        g1ea[whichEa] = m_Euler[3 * feature1 + whichEa];
        g2ea[whichEa] = m_Euler[3 * feature2 + whichEa];
      }

      auto oMatrix1 = OrientationTransformation::eu2om<OrientationD, OrientationD>(OrientationD(g1ea[0], g1ea[1], g1ea[2]));
      auto oMatrix2 = OrientationTransformation::eu2om<OrientationD, OrientationD>(OrientationD(g2ea[0], g2ea[1], g2ea[2]));

      for(int j = 0; j < m_NSym; j++)
      {
        // rotate g1 by symOp
        g1s = EbsdLibMatrixToEigenMatrix(m_OrientationOps[m_Crystal]->getMatSymOpD(j)) * OrientationMatrixToGMatrix(oMatrix1);
        // get the crystal directions along the triangle normals
        normalGrain1 = g1s * normalLab;

        for(int k = 0; k < m_NSym; k++)
        {
          // calculate the symmetric mis orientation
          // rotate g2 by symOp
          g2s = EbsdLibMatrixToEigenMatrix(m_OrientationOps[m_Crystal]->getMatSymOpD(k)) * OrientationMatrixToGMatrix(oMatrix2);
          // transpose rotated g2
          // calculate delta g
          dg = g1s * g2s.transpose(); // dg -- the mis orientation between adjacent grains
          dgT = dg.transpose();

          for(int32 transpose = 0; transpose <= 1; transpose++)
          {
            // check if dg is close to gFix
            if(transpose == 0)
            {
              diffFromFixed = dg * m_GFixedT;
            }
            else
            {
              diffFromFixed = dgT * m_GFixedT;
            }

            const float64 diffAngle = std::acos((diffFromFixed(0, 0) + diffFromFixed(1, 1) + diffFromFixed(2, 2) - 1.0f) * 0.5f);

            if(diffAngle < m_MisResolution)
            {
              normalGrain2 = dgT * normalGrain1; // minus sign before normal_grain2 will be added later

              if(transpose == 0)
              {
                m_SelectedTriangles.push_back(TriAreaAndNormals(m_FaceAreas[triIdx], normalGrain1[0], normalGrain1[1], normalGrain1[2], -normalGrain2[0], -normalGrain2[1], -normalGrain2[2]));
              }
              else
              {
                m_SelectedTriangles.push_back(TriAreaAndNormals(m_FaceAreas[triIdx], -normalGrain2[0], -normalGrain2[1], -normalGrain2[2], normalGrain1[0], normalGrain1[1], normalGrain1[2]));
              }
              uniqueTri.insert(triIdx);
            }
          }
        }
      }
    }
  }

  void operator()(const Range& range) const
  {
    select(range.min(), range.max());
  }

private:
  bool m_ExcludeTripleLines;
  const IGeometry::SharedFaceList& m_Triangles;
  const Int8Array& m_NodeTypes;

#ifdef SIMPLNX_ENABLE_MULTICORE
  tbb::concurrent_vector<TriAreaAndNormals>& m_SelectedTriangles;
#else
  std::vector<TriAreaAndNormals>& m_SelectedTriangles;
#endif
  std::vector<int8_t>& m_TriIncluded;
  float64 m_MisResolution;
  int32 m_PhaseOfInterest;
  const Matrix3dR& m_GFixedT;

  LaueOpsContainerType m_OrientationOps;
  uint32 m_Crystal;
  int32 m_NSym;

  const Float32Array& m_Euler;
  const Int32Array& m_Phases;
  const Int32Array& m_FaceLabels;
  const Float64Array& m_FaceNormals;
  const Float64Array& m_FaceAreas;
};

/**
 * @brief The ProbeDistribution class implements a threaded algorithm that determines the distribution values
 * for the GBCD
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
                    float64 planeResolutionSq, float64 totalFaceArea, int32 numDistinctGBs, float64 ballVolume, const Matrix3dR& gFixedT)
  : m_DistributionValues(distributionValues)
  , m_ErrorValues(errorValues)
  , m_SamplePtsX(samplePtsX)
  , m_SamplePtsY(samplePtsY)
  , m_SamplePtsZ(samplePtsZ)
  , m_SelectedTriangles(selectedTriangles)
  , m_PlaneResolutionSq(planeResolutionSq)
  , m_TotalFaceArea(totalFaceArea)
  , m_NumDistinctGBs(numDistinctGBs)
  , m_BallVolume(ballVolume)
  , m_GFixedT(gFixedT)
  {
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
      Eigen::Vector3d fixedNormal1 = {m_SamplePtsX.at(ptIdx), m_SamplePtsY.at(ptIdx), m_SamplePtsZ.at(ptIdx)};
      Eigen::Vector3d fixedNormal2 = m_GFixedT * fixedNormal1;

      for(const auto& selectedTriangle : m_SelectedTriangles)
      {
        for(int32 inversion = 0; inversion <= 1; inversion++)
        {
          float64 sign = 1.0f;
          if(inversion == 1)
          {
            sign = -1.0f;
          }

          const float64 theta1 =
              std::acos(sign * (selectedTriangle.normalGrain1X * fixedNormal1[0] + selectedTriangle.normalGrain1Y * fixedNormal1[1] + selectedTriangle.normalGrain1Z * fixedNormal1[2]));
          const float64 theta2 =
              std::acos(-sign * (selectedTriangle.normalGrain2X * fixedNormal2[0] + selectedTriangle.normalGrain2Y * fixedNormal2[1] + selectedTriangle.normalGrain2Z * fixedNormal2[2]));
          const float64 distSq = 0.5f * (theta1 * theta1 + theta2 * theta2);

          if(distSq < m_PlaneResolutionSq)
          {
            m_DistributionValues[ptIdx] += selectedTriangle.Area;
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
  float64 m_PlaneResolutionSq;
  float64 m_TotalFaceArea;
  int32 m_NumDistinctGBs;
  float64 m_BallVolume;
  Matrix3dR m_GFixedT;
};

} // namespace GBCDMetricBased

// -----------------------------------------------------------------------------
FindGBCDMetricBased::FindGBCDMetricBased(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindGBCDMetricBasedInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindGBCDMetricBased::~FindGBCDMetricBased() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindGBCDMetricBased::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindGBCDMetricBased::operator()()
{
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

  // -------------------- set resolutions and 'ball volumes' based on user's selection -------------
  float64 misResolution = k_ResolutionChoices[m_InputValues->ChosenLimitDists][0];
  float64 planeResolution = k_ResolutionChoices[m_InputValues->ChosenLimitDists][1];

  misResolution *= Constants::k_PiOver180F;
  planeResolution *= Constants::k_PiOver180F;
  const float64 planeResolutionSq = planeResolution * planeResolution;

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

  // ------------------- before computing the distribution, we must find normalization factors -----
  float64 ballVolume = k_BallVolumesM3M[m_InputValues->ChosenLimitDists];
  {
    std::vector<LaueOps::Pointer> ops = LaueOps::GetAllOrientationOps();
    auto crystalStruct = static_cast<int32>(crystalStructures[m_InputValues->PhaseOfInterest]);
    const int32 nSym = ops[crystalStruct]->getNumSymOps();

    if(crystalStruct != 1)
    {
      auto symFactor = static_cast<float64>(nSym) / 24.0;
      symFactor *= symFactor;
      ballVolume *= symFactor;
    }
  }

  // ------------------------------ generation of sampling points ----------------------------------
  m_MessageHandler(IFilter::Message::Type::Info, "Generating sampling points");

  // generate "Golden Section Spiral", see http://www.softimageblog.com/archives/115
  const int32 numSamplePtsWholeSphere = 2 * m_InputValues->NumSamplPts; // here we generate points on the whole sphere
  std::vector<float64> samplePtsX(0);
  std::vector<float64> samplePtsY(0);
  std::vector<float64> samplePtsZ(0);

  const float64 off = 2.0 / static_cast<float64>(numSamplePtsWholeSphere);
  const float64 kConst1 = Constants::k_PiD * (3.0 - std::sqrt(5.0));
  for(int32 ptIdxWholeSph = 0; ptIdxWholeSph < numSamplePtsWholeSphere; ptIdxWholeSph++)
  {
    if(getCancel())
    {
      return {};
    }

    const float64 y = (static_cast<float64>(ptIdxWholeSph) * off) - 1.0 + (0.5 * off);
    const float64 r = std::sqrt(std::fmax(1.0 - y * y, 0.0));
    const float64 phi = static_cast<float64>(ptIdxWholeSph) * kConst1;

    const float64 z = std::sin(phi) * r;

    if(z > 0.0)
    {
      samplePtsX.push_back(std::cos(phi) * r);
      samplePtsY.push_back(y);
      samplePtsZ.push_back(z);
    }
  }

  // Add points at the equator for better performance of some plotting tools
  for(float64 phi = 0.0; phi <= Constants::k_2PiD; phi += planeResolution)
  {
    samplePtsX.push_back(std::cos(phi));
    samplePtsY.push_back(std::sin(phi));
    samplePtsZ.push_back(0.0);
  }

  // Convert axis angle to matrix representation of mis orientation
  auto gFixedAngle = m_InputValues->MisorientationRotation[3] * Constants::k_PiOver180F;
  Eigen::Vector3d gFixedAxis = {m_InputValues->MisorientationRotation[0], m_InputValues->MisorientationRotation[1], m_InputValues->MisorientationRotation[2]};
  gFixedAxis.normalize();
  auto oMatrix = OrientationTransformation::ax2om<OrientationD, OrientationD>(OrientationD(gFixedAxis[0], gFixedAxis[1], gFixedAxis[2], gFixedAngle));
  const Matrix3dR gFixedT = OrientationMatrixToGMatrixTranspose(oMatrix);

  const usize numMeshTriangles = faceAreas.getNumberOfTuples();

// ---------  find triangles (and equivalent crystallographic parameters) with +- the fixed mis orientation ---------
#ifdef SIMPLNX_ENABLE_MULTICORE
  tbb::concurrent_vector<GBCDMetricBased::TriAreaAndNormals> selectedTriangles(0);
#else
  std::vector<GBCDMetricBased::TriAreaAndNormals> selectedTriangles(0);
#endif

  std::vector<int8> triIncluded(numMeshTriangles, 0);

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
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Step 1/2: Selecting Triangles with the Specified Misorientation ({}% completed)",
                                                               static_cast<int32>(100.0 * static_cast<float64>(i) / static_cast<float64>(numMeshTriangles))));
    if(i + triChunkSize >= numMeshTriangles)
    {
      triChunkSize = numMeshTriangles - i;
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(i, i + triChunkSize);
    dataAlg.setParallelizationEnabled(true);
    dataAlg.execute(GBCDMetricBased::TrianglesSelector(m_InputValues->ExcludeTripleLines, triangles, nodeTypes, selectedTriangles, triIncluded, misResolution, m_InputValues->PhaseOfInterest, gFixedT,
                                                       crystalStructures, eulerAngles, phases, faceLabels, faceNormals, faceAreas));
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
  for(usize triIdx = 0; triIdx < numMeshTriangles; triIdx++)
  {
    totalFaceArea += faceAreas[triIdx] * static_cast<float64>(triIncluded.at(triIdx));
  }

  std::vector<float64> distributionValues(samplePtsX.size(), 0.0);
  std::vector<float64> errorValues(samplePtsX.size(), 0.0);

  usize pointsChunkSize = 100;
  if(samplePtsX.size() < pointsChunkSize)
  {
    pointsChunkSize = samplePtsX.size();
  }

  for(usize i = 0; i < samplePtsX.size(); i += pointsChunkSize)
  {
    if(getCancel())
    {
      return {};
    }
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Step 2/2: Computing Distribution Values at the Section of Interest ({}% completed)",
                                                               static_cast<int32>(100.0 * static_cast<float64>(i) / static_cast<float64>(samplePtsX.size()))));
    if(i + pointsChunkSize >= samplePtsX.size())
    {
      pointsChunkSize = samplePtsX.size() - i;
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(i, i + pointsChunkSize);
    dataAlg.setParallelizationEnabled(true);
    dataAlg.execute(GBCDMetricBased::ProbeDistribution(distributionValues, errorValues, samplePtsX, samplePtsY, samplePtsZ, selectedTriangles, planeResolutionSq, totalFaceArea, numDistinctGBs,
                                                       ballVolume, gFixedT));
  }

  // ------------------------------------------- writing the output --------------------------------

  std::ofstream distributionOutStream(distributionOutput, std::ios_base::out);
  if(!distributionOutStream.is_open())
  {
    return MakeErrorResult(-7237, fmt::format("Error creating GBCD Metric distribution output file {}", distributionOutput.string()));
  }

  std::ofstream errorOutStream(errorOutput, std::ios_base::out);
  if(!errorOutStream.is_open())
  {
    return MakeErrorResult(-7238, fmt::format("Error creating GBCD Metric distribution errors output file {}", errorOutput.string()));
  }

  std::string outputString = fmt::format("{:.1F} {:.1F} {:.1F} {:.1F}\n", m_InputValues->MisorientationRotation[0], m_InputValues->MisorientationRotation[1], m_InputValues->MisorientationRotation[2],
                                         m_InputValues->MisorientationRotation[3]);
  distributionOutStream << outputString;
  errorOutStream << outputString;

  for(usize ptIdx = 0; ptIdx < samplePtsX.size(); ptIdx++)
  {
    const float64 zenith = std::acos(samplePtsZ.at(ptIdx));
    const float64 azimuth = std::atan2(samplePtsY.at(ptIdx), samplePtsX.at(ptIdx));

    const float64 zenithDeg = Constants::k_180OverPiD * zenith;
    const float64 azimuthDeg = Constants::k_180OverPiD * azimuth;

    outputString = fmt::format("{:1.8E} {:1.8E} {:1.8E}\n", azimuthDeg, 90.0 - zenithDeg, distributionValues[ptIdx]);
    distributionOutStream << outputString;

    if(!m_InputValues->SaveRelativeErr)
    {
      outputString = fmt::format("{:1.8E} {:1.8E} {:1.8E}\n", azimuthDeg, 90.0 - zenithDeg, errorValues[ptIdx]);
      errorOutStream << outputString;
    }
    else
    {
      float64 saneErr = 100.0;
      if(distributionValues[ptIdx] > 1e-10)
      {
        saneErr = fmin(100.0, 100.0 * errorValues[ptIdx] / distributionValues[ptIdx]);
      }
      outputString = fmt::format("{:1.8E} {:1.8E} {:1.8E}\n", azimuthDeg, 90.0 - zenithDeg, saneErr);
      errorOutStream << outputString;
    }
  }
  distributionOutStream.close();
  errorOutStream.close();

  return {};
}
