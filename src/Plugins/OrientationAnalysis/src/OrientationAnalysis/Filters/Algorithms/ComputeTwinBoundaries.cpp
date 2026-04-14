#include "ComputeTwinBoundaries.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/OrientationFwd.hpp>

#include <Eigen/Dense>

#include <nonstd/span.hpp>
#include <numbers>

using namespace nx::core;

namespace
{
template <typename T>
bool IsTwinBoundary(const Eigen::Quaternion<T>& quat1, const Eigen::Quaternion<T>& quat2, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps, uint32 laueClass, float32 angTolerance,
                    float32 axisTolerance)
{
  T real = std::numeric_limits<T>::max();
  T axisdiff111;
  T angdiff60;
  T xVal;
  T yVal;
  T zVal;

  Eigen::Quaternion<T> misq;
  Eigen::Quaternion<T> sym_q;
  Eigen::Quaternion<T> s1_misq;
  Eigen::Quaternion<T> s2_misq;

  const int32 nsym = orientationOps[laueClass]->getNumSymOps();
  const Eigen::Quaternion<T> q2 = quat2.conjugate();
  misq = quat1 * q2;

  for(int32 j = 0; j < nsym; j++)
  {
    ebsdlib::Quaternion<T> jQuat = orientationOps[laueClass]->getQuatSymOp(j);
    sym_q = Eigen::Quaterniond(jQuat.w(), jQuat.x(), jQuat.y(), jQuat.z());

    s1_misq = misq * sym_q;

    for(int32 k = 0; k < nsym; k++)
    {
      ebsdlib::Quaternion<T> kQuat = orientationOps[laueClass]->getQuatSymOp(k);
      sym_q = Eigen::Quaterniond(kQuat.w(), kQuat.x(), kQuat.y(), kQuat.z());
      sym_q = sym_q.conjugate();
      s2_misq = sym_q * s1_misq;

      ebsdlib::Quaternion<T> quat(s2_misq.coeffs());
      ebsdlib::AxisAngleDType ax = quat.toAxisAngle();

      xVal = ax[0];
      yVal = ax[1];
      zVal = ax[2];
      real = ax[3];

      real = real * 180.0f / nx::core::Constants::k_PiD;
      axisdiff111 = acos((std::abs(xVal) * std::numbers::inv_sqrt3_v<T>)+(std::abs(yVal) * std::numbers::inv_sqrt3_v<T>)+(std::abs(zVal) * std::numbers::inv_sqrt3_v<T>));
      angdiff60 = std::abs(real - 60.0f);
      if(axisdiff111 < axisTolerance && angdiff60 < angTolerance)
      {
        return true;
      }
    }
  }

  return false;
}

template <typename T>
std::optional<T> FindTwinBoundaryIncoherence(const Eigen::Vector3d& xstl_norm, const Eigen::Quaternion<T>& quat1, const Eigen::Quaternion<T>& quat2,
                                             const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps, uint32 laueClass, float32 angTolerance, float32 axisTolerance)
{
  T real = std::numeric_limits<T>::max();
  T axisdiff111;
  T angdiff60;
  T xVal;
  T yVal;
  T zVal;
  T minIncoherence = 180.0;

  Eigen::Quaternion<T> misq;
  Eigen::Quaternion<T> j_sym_q;
  Eigen::Quaternion<T> sym_q;
  Eigen::Quaternion<T> s1_misq;
  Eigen::Quaternion<T> s2_misq;

  const int32 nsym = orientationOps[laueClass]->getNumSymOps();
  const Eigen::Quaternion<T> q2 = quat2.conjugate();
  misq = quat1 * q2;

  bool valid = false;
  for(int32 j = 0; j < nsym; j++)
  {
    ebsdlib::Quaternion<T> jQuat = orientationOps[laueClass]->getQuatSymOp(j);
    j_sym_q = Eigen::Quaterniond(jQuat.w(), jQuat.x(), jQuat.y(), jQuat.z());

    s1_misq = misq * j_sym_q;

    for(int32 k = 0; k < nsym; k++)
    {
      ebsdlib::Quaternion<T> kQuat = orientationOps[laueClass]->getQuatSymOp(k);
      sym_q = Eigen::Quaterniond(kQuat.w(), kQuat.x(), kQuat.y(), kQuat.z());
      sym_q = sym_q.conjugate();
      s2_misq = sym_q * s1_misq;

      ebsdlib::Quaternion<T> quat(s2_misq.coeffs());
      ebsdlib::AxisAngleDType ax = quat.toAxisAngle();

      xVal = ax[0];
      yVal = ax[1];
      zVal = ax[2];
      real = ax[3];

      real = real * 180.0f / nx::core::Constants::k_PiD;
      axisdiff111 = acos((std::abs(xVal) * std::numbers::inv_sqrt3_v<T>)+(std::abs(yVal) * std::numbers::inv_sqrt3_v<T>)+(std::abs(zVal) * std::numbers::inv_sqrt3_v<T>));
      angdiff60 = std::abs(real - 60.0f);
      if(axisdiff111 < axisTolerance && angdiff60 < angTolerance)
      {
        const Eigen::Vector3d axVec{xVal, yVal, zVal};
        const Eigen::Vector3d s_xstl_norm = j_sym_q.conjugate()._transformVector(xstl_norm);

        T incoherence = 180.0 * std::acos(GeometryMath::CosThetaBetweenVectors(axVec, s_xstl_norm)) / nx::core::Constants::k_PiD;
        if(incoherence > 90.0)
        {
          incoherence = 180.0 - incoherence;
        }
        if(incoherence < minIncoherence)
        {
          minIncoherence = incoherence;
          valid = true;
        }
      }
    }
  }

  if(valid)
  {
    return {minIncoherence};
  }

  return {};
}

/**
 * @brief Parallel worker that identifies twin boundaries and computes their
 * incoherence. All input arrays are passed as local std::vector references
 * (pre-cached from DataStores), eliminating OOC virtual dispatch in the hot
 * loop. Output is written to local uint8/float32 vectors that are later
 * bulk-copied back to DataStores.
 */
class CalculateTwinBoundaryWithIncoherenceImpl
{
  using Matrix3x3 = Eigen::Matrix<float64, 3, 3, Eigen::RowMajor>;

public:
  CalculateTwinBoundaryWithIncoherenceImpl(float32 angtol, float32 axistol, const std::vector<int32>& faceLabels, const std::vector<float64>& faceNormals, const std::vector<float32>& avgQuats,
                                           const std::vector<int32>& featurePhases, const std::vector<uint32>& crystalStructures, std::vector<uint8>& twinBoundariesOut,
                                           std::vector<float32>& twinBoundaryIncoherenceOut, const std::atomic_bool& shouldCancel, std::atomic_bool& hasNaN)
  : m_AxisTol(axistol)
  , m_AngTol(angtol)
  , m_FaceLabels(faceLabels)
  , m_FaceNormals(faceNormals)
  , m_AvgQuats(avgQuats)
  , m_FeaturePhases(featurePhases)
  , m_CrystalStructures(crystalStructures)
  , m_TwinBoundariesOut(twinBoundariesOut)
  , m_TwinBoundaryIncoherenceOut(twinBoundaryIncoherenceOut)
  , m_ShouldCancel(shouldCancel)
  , m_HasNaN(hasNaN)
  , m_OrientationOps(ebsdlib::LaueOps::GetAllOrientationOps())
  {
  }

  void generate(usize start, usize end) const
  {
    for(usize i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const int32 feature1 = m_FaceLabels[2 * i];
      const int32 feature2 = m_FaceLabels[2 * i + 1];
      if(feature1 > 0 && feature2 > 0 && m_FeaturePhases[feature1] == m_FeaturePhases[feature2])
      {
        const uint32 crystalStructure = m_CrystalStructures[m_FeaturePhases[feature1]];
        if(crystalStructure != ebsdlib::CrystalStructure::Cubic_High && crystalStructure != ebsdlib::CrystalStructure::Cubic_Low)
        {
          continue;
        }

        const Eigen::Quaterniond q1(m_AvgQuats[(feature1 * 4) + 3], m_AvgQuats[feature1 * 4], m_AvgQuats[(feature1 * 4) + 1], m_AvgQuats[(feature1 * 4) + 2]);
        const Eigen::Quaterniond q2(m_AvgQuats[(feature2 * 4) + 3], m_AvgQuats[feature2 * 4], m_AvgQuats[(feature2 * 4) + 1], m_AvgQuats[(feature2 * 4) + 2]);

        const Matrix3x3 orientationMatrix = q1.matrix().transpose();
        const Eigen::Vector3d normals{m_FaceNormals[3 * i], m_FaceNormals[3 * i + 1], m_FaceNormals[3 * i + 2]};
        const Eigen::Vector3d xstl_norm = normals.transpose() * orientationMatrix;

        if(normals.hasNaN())
        {
          m_HasNaN.store(true);
          continue;
        }

        std::optional<float64> minIncoherence = FindTwinBoundaryIncoherence(xstl_norm, q1, q2, m_OrientationOps, crystalStructure, m_AngTol, m_AxisTol);

        if(minIncoherence.has_value())
        {
          m_TwinBoundariesOut[i] = 1;
          m_TwinBoundaryIncoherenceOut[i] = static_cast<float32>(minIncoherence.value());
        }
      }
    }
  }

  void operator()(const nx::core::Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  float32 m_AxisTol;
  float32 m_AngTol;
  const std::vector<int32>& m_FaceLabels;
  const std::vector<float64>& m_FaceNormals;
  const std::vector<float32>& m_AvgQuats;
  const std::vector<int32>& m_FeaturePhases;
  const std::vector<uint32>& m_CrystalStructures;
  std::vector<uint8>& m_TwinBoundariesOut;
  std::vector<float32>& m_TwinBoundaryIncoherenceOut;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_bool& m_HasNaN;
  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;
};

/**
 * @brief Parallel worker that identifies twin boundaries (without computing
 * incoherence). All input arrays are local std::vector references, avoiding
 * OOC DataStore access during parallel execution. Output flags are written to
 * a local uint8 vector.
 */
class CalculateTwinBoundaryImpl
{
public:
  CalculateTwinBoundaryImpl(float32 angtol, float32 axistol, const std::vector<int32>& faceLabels, const std::vector<float32>& avgQuats, const std::vector<int32>& featurePhases,
                            const std::vector<uint32>& crystalStructures, std::vector<uint8>& twinBoundariesOut, const std::atomic_bool& shouldCancel)
  : m_AxisTol(axistol)
  , m_AngTol(angtol)
  , m_FaceLabels(faceLabels)
  , m_AvgQuats(avgQuats)
  , m_FeaturePhases(featurePhases)
  , m_CrystalStructures(crystalStructures)
  , m_TwinBoundariesOut(twinBoundariesOut)
  , m_ShouldCancel(shouldCancel)
  , m_OrientationOps(ebsdlib::LaueOps::GetAllOrientationOps())
  {
  }

  void generate(usize start, usize end) const
  {
    for(usize i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const int32 feature1 = m_FaceLabels[2 * i];
      const int32 feature2 = m_FaceLabels[2 * i + 1];
      if(feature1 > 0 && feature2 > 0 && m_FeaturePhases[feature1] == m_FeaturePhases[feature2])
      {
        const uint32 crystalStructure = m_CrystalStructures[m_FeaturePhases[feature1]];
        if(crystalStructure != ebsdlib::CrystalStructure::Cubic_High && crystalStructure != ebsdlib::CrystalStructure::Cubic_Low)
        {
          continue;
        }

        const Eigen::Quaterniond q1(m_AvgQuats[(feature1 * 4) + 3], m_AvgQuats[feature1 * 4], m_AvgQuats[(feature1 * 4) + 1], m_AvgQuats[(feature1 * 4) + 2]);
        const Eigen::Quaterniond q2(m_AvgQuats[(feature2 * 4) + 3], m_AvgQuats[feature2 * 4], m_AvgQuats[(feature2 * 4) + 1], m_AvgQuats[(feature2 * 4) + 2]);

        if(IsTwinBoundary(q1, q2, m_OrientationOps, crystalStructure, m_AngTol, m_AxisTol))
        {
          m_TwinBoundariesOut[i] = 1;
        }
      }
    }
  }

  void operator()(const nx::core::Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  float32 m_AxisTol;
  float32 m_AngTol;
  const std::vector<int32>& m_FaceLabels;
  const std::vector<float32>& m_AvgQuats;
  const std::vector<int32>& m_FeaturePhases;
  const std::vector<uint32>& m_CrystalStructures;
  std::vector<uint8>& m_TwinBoundariesOut;
  const std::atomic_bool& m_ShouldCancel;
  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeTwinBoundaries::ComputeTwinBoundaries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeTwinBoundariesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeTwinBoundaries::~ComputeTwinBoundaries() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeTwinBoundaries::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
/**
 * @brief Identifies twin boundaries on a triangle surface mesh by checking
 * misorientation between adjacent grains against the 60-degree <111> twin
 * relationship. Optionally computes the boundary incoherence angle.
 *
 * OOC strategy: All arrays (ensemble, feature, and face level) are bulk-read
 * into local std::vectors via copyIntoBuffer before the parallel computation
 * begins. The parallel workers operate entirely on these local caches with
 * zero OOC virtual dispatch. After parallel execution, results are bulk-written
 * back to DataStores via copyFromBuffer.
 */
Result<> ComputeTwinBoundaries::operator()()
{
  // -------------------------------------------------------------------------
  // Bulk-read ensemble-level crystalStructures into local memory (tiny array).
  // -------------------------------------------------------------------------
  const auto& crystalStructuresStore = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();
  const usize numCrystalStructures = crystalStructuresStore.getNumberOfTuples();
  std::vector<uint32> crystalStructures(numCrystalStructures);
  crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), numCrystalStructures));

  bool allPhasesCubic = true;
  bool noPhasesCubic = true;
  for(usize i = 1; i < numCrystalStructures; ++i)
  {
    const auto crystalStructureType = crystalStructures[i];
    const bool isCubic = crystalStructureType == ebsdlib::CrystalStructure::Cubic_High || crystalStructureType == ebsdlib::CrystalStructure::Cubic_Low;
    allPhasesCubic = allPhasesCubic && isCubic;
    noPhasesCubic = noPhasesCubic && !isCubic;
  }

  if(noPhasesCubic)
  {
    return MakeErrorResult(-93210, "Finding the twin boundaries requires at least one phase to be Cubic-Low m-3 or Cubic-High m-3m type crystal structures but none were found.");
  }

  Result<> result;
  if(!allPhasesCubic)
  {
    result.warnings().push_back({-93211, "Finding the twin boundaries requires Cubic-Low m-3 or Cubic-High m-3m type crystal structures. Calculations for non Cubic phases will be skipped."});
  }

  // -------------------------------------------------------------------------
  // Bulk-read feature-level arrays into local vectors (O(features)). These are
  // accessed randomly by feature ID during the parallel face loop.
  // -------------------------------------------------------------------------
  const auto& featurePhasesStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath)->getDataStoreRef();
  const usize numFeatures = featurePhasesStore.getNumberOfTuples();
  std::vector<int32> featurePhases(numFeatures);
  featurePhasesStore.copyIntoBuffer(0, nonstd::span<int32>(featurePhases.data(), numFeatures));

  const auto& avgQuatsStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgQuatsArrayPath)->getDataStoreRef();
  std::vector<float32> avgQuats(numFeatures * 4);
  avgQuatsStore.copyIntoBuffer(0, nonstd::span<float32>(avgQuats.data(), numFeatures * 4));

  // -------------------------------------------------------------------------
  // Bulk-read face-level arrays into local vectors (O(faces), scales with
  // surface area rather than volume). This is the largest cache but still
  // much smaller than cell-level data in a typical EBSD dataset.
  // -------------------------------------------------------------------------
  const auto& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsArrayPath)->getDataStoreRef();
  const usize numFaces = faceLabelsStore.getNumberOfTuples();

  std::vector<int32> faceLabels(numFaces * 2);
  faceLabelsStore.copyIntoBuffer(0, nonstd::span<int32>(faceLabels.data(), numFaces * 2));

  std::vector<float64> faceNormals;
  if(m_InputValues->FindCoherence)
  {
    const auto& faceNormalsStore = m_DataStructure.getDataAs<Float64Array>(m_InputValues->FaceNormalsArrayPath)->getDataStoreRef();
    faceNormals.resize(numFaces * 3);
    faceNormalsStore.copyIntoBuffer(0, nonstd::span<float64>(faceNormals.data(), numFaces * 3));
  }

  // -------------------------------------------------------------------------
  // Output buffers — parallel workers write directly into these local vectors.
  // After execution completes, results are bulk-copied to the output DataStores.
  // -------------------------------------------------------------------------
  std::vector<uint8> twinBoundariesOut(numFaces, 0);
  std::vector<float32> twinBoundaryIncoherenceOut;
  if(m_InputValues->FindCoherence)
  {
    twinBoundaryIncoherenceOut.resize(numFaces, 180.0f);
  }

  const float32 angtol = m_InputValues->AngleTolerance;
  const float32 axistol = m_InputValues->AxisTolerance * Constants::k_PiF / 180.0f;

  // -------------------------------------------------------------------------
  // Parallel execution over all faces. The workers index only into local
  // vectors, so there is zero OOC DataStore access during computation.
  // -------------------------------------------------------------------------
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numFaces);

  std::atomic_bool hasNaN = false;
  if(m_InputValues->FindCoherence)
  {
    dataAlg.execute(CalculateTwinBoundaryWithIncoherenceImpl(angtol, axistol, faceLabels, faceNormals, avgQuats, featurePhases, crystalStructures, twinBoundariesOut, twinBoundaryIncoherenceOut,
                                                             m_ShouldCancel, hasNaN));
  }
  else
  {
    dataAlg.execute(CalculateTwinBoundaryImpl(angtol, axistol, faceLabels, avgQuats, featurePhases, crystalStructures, twinBoundariesOut, m_ShouldCancel));
  }

  // -------------------------------------------------------------------------
  // Write results from local buffers back to DataStores via bulk I/O.
  // TwinBoundaries uses a MaskCompare interface (no bulk copy API), so it
  // must be written element-by-element. The incoherence array uses copyFromBuffer.
  // -------------------------------------------------------------------------
  std::unique_ptr<MaskCompareUtilities::MaskCompare> twinBoundaries;
  try
  {
    twinBoundaries = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->TwinBoundariesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    return MakeErrorResult(-93212, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->TwinBoundariesArrayPath.toString()));
  }

  // TwinBoundaries is a MaskCompare — must write per-element (no bulk API)
  for(usize i = 0; i < numFaces; i++)
  {
    if(twinBoundariesOut[i])
    {
      twinBoundaries->setValue(i, true);
    }
  }

  if(m_InputValues->FindCoherence)
  {
    auto& incoherenceStore = m_DataStructure.getDataAs<Float32Array>(m_InputValues->TwinBoundaryIncoherenceArrayPath)->getDataStoreRef();
    incoherenceStore.copyFromBuffer(0, nonstd::span<const float32>(twinBoundaryIncoherenceOut.data(), numFaces));
  }

  if(m_InputValues->FindCoherence && hasNaN.load())
  {
    return MakeWarningVoidResult(-93213, fmt::format("NaNs were detected in the normals array ({}). These values were marked false.", m_InputValues->FaceNormalsArrayPath.toString()));
  }

  return result;
}
