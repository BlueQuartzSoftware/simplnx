#include "ComputeTwinBoundaries.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

#include <Eigen/Dense>

#include <numbers>

using namespace nx::core;

namespace
{
template <typename T>
bool IsTwinBoundary(const Eigen::Quaternion<T>& quat1, const Eigen::Quaternion<T>& quat2, const std::vector<LaueOps::Pointer>& orientationOps, uint32 crystalStructure, float32 angTolerance,
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

  const int32 nsym = orientationOps[crystalStructure]->getNumSymOps();
  const Eigen::Quaternion<T> q2 = quat2.conjugate();
  misq = quat1 * q2;

  for(int32 j = 0; j < nsym; j++)
  {
    Quaternion<T> jQuat = orientationOps[crystalStructure]->getQuatSymOp(j);
    sym_q = Eigen::Quaterniond(jQuat.w(), jQuat.x(), jQuat.y(), jQuat.z());

    // calculate crystal direction parallel to normal
    s1_misq = misq * sym_q;

    for(int32 k = 0; k < nsym; k++)
    {
      // calculate the symmetric misorienation
      Quaternion<T> kQuat = orientationOps[crystalStructure]->getQuatSymOp(k);
      sym_q = Eigen::Quaterniond(kQuat.w(), kQuat.x(), kQuat.y(), kQuat.z());
      sym_q = sym_q.conjugate();
      s2_misq = sym_q * s1_misq;

      OrientationTransformation::qu2ax<Eigen::Vector4<T>, Orientation<T>>(s2_misq.coeffs(), Quaternion<T>::Order::VectorScalar).toAxisAngle(xVal, yVal, zVal, real);

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
                                             const std::vector<LaueOps::Pointer>& orientationOps, uint32 crystalStructure, float32 angTolerance, float32 axisTolerance)
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

  const int32 nsym = orientationOps[crystalStructure]->getNumSymOps();
  const Eigen::Quaternion<T> q2 = quat2.conjugate();
  misq = quat1 * q2;

  bool valid = false;
  for(int32 j = 0; j < nsym; j++)
  {
    Quaternion<T> jQuat = orientationOps[crystalStructure]->getQuatSymOp(j);
    j_sym_q = Eigen::Quaterniond(jQuat.w(), jQuat.x(), jQuat.y(), jQuat.z());

    // calculate crystal direction parallel to normal
    s1_misq = misq * j_sym_q;

    for(int32 k = 0; k < nsym; k++)
    {
      // calculate the symmetric misorienation
      Quaternion<T> kQuat = orientationOps[crystalStructure]->getQuatSymOp(k);
      sym_q = Eigen::Quaterniond(kQuat.w(), kQuat.x(), kQuat.y(), kQuat.z());
      sym_q = sym_q.conjugate();
      s2_misq = sym_q * s1_misq;

      OrientationTransformation::qu2ax<Eigen::Vector4<T>, Orientation<T>>(s2_misq.coeffs(), Quaternion<T>::Order::VectorScalar).toAxisAngle(xVal, yVal, zVal, real);

      real = real * 180.0f / nx::core::Constants::k_PiD;
      axisdiff111 = acos((std::abs(xVal) * std::numbers::inv_sqrt3_v<T>)+(std::abs(yVal) * std::numbers::inv_sqrt3_v<T>)+(std::abs(zVal) * std::numbers::inv_sqrt3_v<T>));
      angdiff60 = std::abs(real - 60.0f);
      if(axisdiff111 < axisTolerance && angdiff60 < angTolerance)
      {
        const Eigen::Vector3d axVec{xVal, yVal, zVal};
        const Eigen::Vector3d s_xstl_norm = j_sym_q.conjugate()._transformVector(xstl_norm); // conjugate for active rotate

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
 * @brief The CalculateTwinBoundaryImpl class implements a threaded algorithm that determines whether a boundary is twin related and calculates
 * the respective incoherence. The calculations are performed on a surface mesh.
 */
class CalculateTwinBoundaryWithIncoherenceImpl
{
  using Matrix3x3 = Eigen::Matrix<float64, 3, 3, Eigen::RowMajor>;

public:
  CalculateTwinBoundaryWithIncoherenceImpl(float32 angtol, float32 axistol, const Int32AbstractDataStore& faceLabels, const Float64AbstractDataStore& faceNormals,
                                           const Float32AbstractDataStore& avgQuats, const Int32AbstractDataStore& featurePhases, const UInt32AbstractDataStore& crystalStructures,
                                           std::unique_ptr<MaskCompare>& twinBoundaries, Float32AbstractDataStore& twinBoundaryIncoherence, const std::atomic_bool& shouldCancel)
  : m_AxisTol(axistol)
  , m_AngTol(angtol)
  , m_FaceLabels(faceLabels)
  , m_FaceNormals(faceNormals)
  , m_FeaturePhases(featurePhases)
  , m_AvgQuats(avgQuats)
  , m_TwinBoundaries(twinBoundaries)
  , m_TwinBoundaryIncoherence(twinBoundaryIncoherence)
  , m_CrystalStructures(crystalStructures)
  , m_ShouldCancel(shouldCancel)
  , m_OrientationOps(LaueOps::GetAllOrientationOps())
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
      const int32 feature2 = m_FaceLabels[(2 * i) + 1];
      const int32 phase = m_FeaturePhases[feature1]; // Feature1 was arbitrarily selected the feature phase index is identical
      if(feature1 > 0 && feature2 > 0 && phase == m_FeaturePhases[feature2])
      {
        const uint32 crystalStructure = m_CrystalStructures[phase];

        // Avg Quats is stored Vector Scalar but the Quaternion Constructor is Scalar-Vector
        const Eigen::Quaterniond q1(m_AvgQuats[(feature1 * 4) + 3], m_AvgQuats[feature1 * 4], m_AvgQuats[(feature1 * 4) + 1], m_AvgQuats[(feature1 * 4) + 2]); // W X Y Z
        const Eigen::Quaterniond q2(m_AvgQuats[(feature2 * 4) + 3], m_AvgQuats[feature2 * 4], m_AvgQuats[(feature2 * 4) + 1], m_AvgQuats[(feature2 * 4) + 2]); // W X Y Z

        const Matrix3x3 orientationMatrix = q1.matrix().transpose();
        const Eigen::Vector3d normals{m_FaceNormals[3 * i], m_FaceNormals[(3 * i) + 1], m_FaceNormals[(3 * i) + 2]};
        const Eigen::Vector3d xstl_norm = normals.transpose() * orientationMatrix;

        if(normals.hasNaN())
        {
          continue;
        }

        std::optional<float64> minIncoherence = FindTwinBoundaryIncoherence(xstl_norm, q1, q2, m_OrientationOps, crystalStructure, m_AngTol, m_AxisTol);

        if(minIncoherence.has_value())
        {
          m_TwinBoundaries->setValue(i, true);
          m_TwinBoundaryIncoherence[i] = static_cast<float32>(minIncoherence.value());
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
  const Int32AbstractDataStore& m_FaceLabels;
  const Float64AbstractDataStore& m_FaceNormals;
  const Float32AbstractDataStore& m_AvgQuats;
  const Int32AbstractDataStore& m_FeaturePhases;
  const UInt32AbstractDataStore& m_CrystalStructures;
  std::unique_ptr<MaskCompare>& m_TwinBoundaries;
  Float32AbstractDataStore& m_TwinBoundaryIncoherence;
  const std::atomic_bool& m_ShouldCancel;
  std::vector<LaueOps::Pointer> m_OrientationOps;
};

/**
 * @brief The CalculateTwinBoundaryImpl class implements a threaded algorithm that determines whether a boundary is twin related.
 * The calculations are performed on a surface mesh.
 */
class CalculateTwinBoundaryImpl
{
public:
  CalculateTwinBoundaryImpl(float32 angtol, float32 axistol, const Int32AbstractDataStore& faceLabels, const Float32AbstractDataStore& avgQuats, const Int32AbstractDataStore& featurePhases,
                            const UInt32AbstractDataStore& crystalStructures, std::unique_ptr<MaskCompare>& twinBoundaries, const std::atomic_bool& shouldCancel)
  : m_AxisTol(axistol)
  , m_AngTol(angtol)
  , m_FaceLabels(faceLabels)
  , m_FeaturePhases(featurePhases)
  , m_AvgQuats(avgQuats)
  , m_TwinBoundaries(twinBoundaries)
  , m_CrystalStructures(crystalStructures)
  , m_ShouldCancel(shouldCancel)
  , m_OrientationOps(LaueOps::GetAllOrientationOps())
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
      const int32 feature2 = m_FaceLabels[(2 * i) + 1];
      const int32 phase = m_FeaturePhases[feature1]; // Feature1 was arbitrarily selected the feature phase index is identical
      if(feature1 > 0 && feature2 > 0 && phase == m_FeaturePhases[feature2])
      {
        const uint32 crystalStructure = m_CrystalStructures[phase];

        // Avg Quats is stored Vector Scalar but the Quaternion Constructor is Scalar-Vector
        const Eigen::Quaterniond q1(m_AvgQuats[(feature1 * 4) + 3], m_AvgQuats[feature1 * 4], m_AvgQuats[(feature1 * 4) + 1], m_AvgQuats[(feature1 * 4) + 2]); // W X Y Z
        const Eigen::Quaterniond q2(m_AvgQuats[(feature2 * 4) + 3], m_AvgQuats[feature2 * 4], m_AvgQuats[(feature2 * 4) + 1], m_AvgQuats[(feature2 * 4) + 2]); // W X Y Z
        m_TwinBoundaries->setValue(i, IsTwinBoundary(q1, q2, m_OrientationOps, crystalStructure, m_AngTol, m_AxisTol));
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
  const Int32AbstractDataStore& m_FaceLabels;
  const Float32AbstractDataStore& m_AvgQuats;
  const Int32AbstractDataStore& m_FeaturePhases;
  const UInt32AbstractDataStore& m_CrystalStructures;
  std::unique_ptr<MaskCompare>& m_TwinBoundaries;
  const std::atomic_bool& m_ShouldCancel;
  std::vector<LaueOps::Pointer> m_OrientationOps;
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
Result<> ComputeTwinBoundaries::operator()()
{
  const auto& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsArrayPath)->getDataStoreRef();
  const auto& avgQuats = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgQuatsArrayPath)->getDataStoreRef();
  const auto& featurePhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath)->getDataStoreRef();
  const auto& crystalStructures = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();

  std::unique_ptr<MaskCompare> twinBoundaries;
  try
  {
    twinBoundaries = InstantiateMaskCompare(m_DataStructure, m_InputValues->TwinBoundariesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    return MakeErrorResult(-54300, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->TwinBoundariesArrayPath.toString()));
  }

  const float32 angtol = m_InputValues->AngleTolerance;
  const float32 axistol = m_InputValues->AxisTolerance * Constants::k_PiF / 180.0f;

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, faceLabels.getNumberOfTuples());
  if(m_InputValues->FindCoherence)
  {
    const auto& faceNormals = m_DataStructure.getDataAs<Float64Array>(m_InputValues->FaceNormalsArrayPath)->getDataStoreRef();
    auto& twinBoundaryIncoherence = m_DataStructure.getDataAs<Float32Array>(m_InputValues->TwinBoundaryIncoherenceArrayPath)->getDataStoreRef();
    twinBoundaryIncoherence.fill(180.0f); // For backwards compatibility
    dataAlg.execute(
        CalculateTwinBoundaryWithIncoherenceImpl(angtol, axistol, faceLabels, faceNormals, avgQuats, featurePhases, crystalStructures, twinBoundaries, twinBoundaryIncoherence, m_ShouldCancel));
  }
  else
  {
    dataAlg.execute(CalculateTwinBoundaryImpl(angtol, axistol, faceLabels, avgQuats, featurePhases, crystalStructures, twinBoundaries, m_ShouldCancel));
  }

  return {};
}
