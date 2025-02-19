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
#include <simplnx/Utilities/Math/MatrixMath.hpp>

using namespace nx::core;

namespace
{
class SIMPLTwinBoundaryImpl
{
  using Matrix3x3 = Eigen::Matrix<float64, 3, 3, Eigen::RowMajor>;

public:
  SIMPLTwinBoundaryImpl(float32 angtol, float32 axistol, const Int32AbstractDataStore& faceLabels, const Float64AbstractDataStore& faceNormals, const Float32AbstractDataStore& avgQuats,
                        const Int32AbstractDataStore& featurePhases, const UInt32AbstractDataStore& crystalStructures, std::unique_ptr<MaskCompare>& twinBoundaries,
                        Float32AbstractDataStore& twinBoundaryIncoherence, const std::atomic_bool& shouldCancel, bool findCoherence)
  : m_AxisTol(axistol)
  , m_AngTol(angtol)
  , m_Labels(faceLabels)
  , m_Normals(faceNormals)
  , m_Phases(featurePhases)
  , m_AvgQuats(avgQuats)
  , m_TwinBoundaries(twinBoundaries)
  , m_TwinBoundaryIncoherence(twinBoundaryIncoherence)
  , m_CrystalStructures(crystalStructures)
  , m_ShouldCancel(shouldCancel)
  , m_FindCoherence(findCoherence)
  , m_OrientationOps(LaueOps::GetAllOrientationOps())
  {
  }
  void generate(size_t start, size_t end) const
  {
    int32_t feature1 = 0, feature2 = 0;
    double normal[3] = {0.0, 0.0, 0.0};
    double g1[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    double w = 0.0;
    uint32_t phase1 = 0, phase2 = 0;

    double axisdiff111 = 0.0, angdiff60 = 0.0;
    double n[3] = {0.0, 0.0, 0.0};
    double incoherence = 0.0;
    double n1 = 0.0, n2 = 0.0, n3 = 0.0;

    QuatD misq;
    QuatD sym_q;
    QuatD s1_misq;
    QuatD s2_misq;

    // QuatF* quats = reinterpret_cast<QuatF*>(m_Quats);

    std::array<double, 3> xstl_norm = {0.0, 0.0, 0.0};
    std::array<double, 3> s_xstl_norm = {0.0, 0.0, 0.0};

    for(size_t i = start; i < end; i++)
    {
      feature1 = m_Labels[2 * i];
      feature2 = m_Labels[2 * i + 1];
      if(m_FindCoherence)
      {
        normal[0] = m_Normals[3 * i];
        normal[1] = m_Normals[3 * i + 1];
        normal[2] = m_Normals[3 * i + 2];
      }
      if(feature1 > 0 && feature2 > 0 && m_Phases[feature1] == m_Phases[feature2])
      {
        w = std::numeric_limits<float>::max();
        QuatD q1(m_AvgQuats[feature1 * 4], m_AvgQuats[(feature1 * 4) + 1], m_AvgQuats[(feature1 * 4) + 2], m_AvgQuats[(feature1 * 4) + 3]); // W X Y Z
        QuatD q2(m_AvgQuats[feature2 * 4], m_AvgQuats[(feature2 * 4) + 1], m_AvgQuats[(feature2 * 4) + 2], m_AvgQuats[(feature2 * 4) + 3]); // W X Y Z

        phase1 = m_CrystalStructures[m_Phases[feature1]];
        phase2 = m_CrystalStructures[m_Phases[feature2]];
        if(phase1 == phase2)
        {
          int32_t nsym = m_OrientationOps[phase1]->getNumSymOps();
          q2 = q2.conjugate();
          misq = q1 * q2;
          OrientationTransformation::qu2om<QuatD, OrientationD>(q1).toGMatrix(g1);

          if(m_FindCoherence)
          {
            MatrixMath::Multiply3x3with3x1(g1, normal, xstl_norm.data());
          }

          if(1835749 == i)
          {
            std::cout << "check";
          }

          for(int32_t j = 0; j < nsym; j++)
          {
            sym_q = m_OrientationOps[phase1]->getQuatSymOp(j);
            // calculate crystal direction parallel to normal
            s1_misq = misq * sym_q;

            if(m_FindCoherence)
            {
              s_xstl_norm = sym_q.multiplyByVector(xstl_norm.data());
            }

            for(int32_t k = 0; k < nsym; k++)
            {
              // calculate the symmetric misorienation
              sym_q = m_OrientationOps[phase1]->getQuatSymOp(k);
              sym_q = sym_q.conjugate();
              s2_misq = sym_q * s1_misq;

              OrientationTransformation::qu2ax<QuatD, OrientationD>(s2_misq).toAxisAngle(n1, n2, n3, w);

              w = w * 180.0f / nx::core::Constants::k_PiD;
              axisdiff111 = acos(std::fabs(n1) * 0.57735f + std::fabs(n2) * 0.57735f + std::fabs(n3) * 0.57735f);
              angdiff60 = std::fabs(w - 60.0f);
              if(axisdiff111 < m_AxisTol && angdiff60 < m_AngTol)
              {
                n[0] = n1;
                n[1] = n2;
                n[2] = n3;
                m_TwinBoundaries->setValue(i,true);
                if(m_FindCoherence)
                {
                  incoherence = 180.0 * std::acos(GeometryMath::CosThetaBetweenVectors(Point3Dd{n}, Point3Dd{s_xstl_norm.data()})) / nx::core::Constants::k_PiD;
                  if(incoherence > 90.0)
                  {
                    incoherence = 180.0 - incoherence;
                  }
                  if(incoherence < m_TwinBoundaryIncoherence[i])
                  {
                    m_TwinBoundaryIncoherence[i] = incoherence;
                  }
                }
              }
            }
          }
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
  const Int32AbstractDataStore& m_Labels;
  const Float64AbstractDataStore& m_Normals;
  const Float32AbstractDataStore& m_AvgQuats;
  const Int32AbstractDataStore& m_Phases;
  const UInt32AbstractDataStore& m_CrystalStructures;
  std::unique_ptr<MaskCompare>& m_TwinBoundaries;
  Float32AbstractDataStore& m_TwinBoundaryIncoherence;
  const std::atomic_bool& m_ShouldCancel;
  bool m_FindCoherence;
  std::vector<LaueOps::Pointer> m_OrientationOps;
};

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

    for(int32 k = j + 1; k < nsym; k++)
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

    for(int32 k = j + 1; k < nsym; k++)
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

        if(1835749 == i)
        {
          std::cout << "check";
        }

        std::optional<float64> minIncoherence = FindTwinBoundaryIncoherence(xstl_norm, q1, q2, m_OrientationOps, crystalStructure, m_AngTol, m_AxisTol);

        if(minIncoherence.has_value())
        {
          m_TwinBoundaries->setValue(i, true);
          if(m_TwinBoundaryIncoherence[i] != minIncoherence.value())
          {
            std::cout << "Index of Discrepancy: " << i << " | Original Value: " << m_TwinBoundaryIncoherence[i] << " | New Value: " << minIncoherence.value() << std::endl;
          }

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
        SIMPLTwinBoundaryImpl(angtol, axistol, faceLabels, faceNormals, avgQuats, featurePhases, crystalStructures, twinBoundaries, twinBoundaryIncoherence, m_ShouldCancel, m_InputValues->FindCoherence));
    dataAlg.execute(
        CalculateTwinBoundaryWithIncoherenceImpl(angtol, axistol, faceLabels, faceNormals, avgQuats, featurePhases, crystalStructures, twinBoundaries, twinBoundaryIncoherence, m_ShouldCancel));
  }
  else
  {
    dataAlg.execute(CalculateTwinBoundaryImpl(angtol, axistol, faceLabels, avgQuats, featurePhases, crystalStructures, twinBoundaries, m_ShouldCancel));
  }

  return {};
}
