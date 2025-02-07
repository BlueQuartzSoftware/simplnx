#include "ComputeTwinBoundaries.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

#include <Eigen/Dense>
#include <numbers>

using namespace nx::core;

namespace
{
// Assumes Vector-Scalar ordering
template <typename T>
Eigen::Vector3<T> MultiplyQuatByVector(const Eigen::Vector4<T>& quaternion, const Eigen::Vector3<T>& multiplier)
{
  constexpr usize k_XIndex = 0;
  constexpr usize k_YIndex = 1;
  constexpr usize k_ZIndex = 2;
  constexpr usize k_WIndex = 3;

  Eigen::Vector4<T> squares = quaternion.array().square();

  T qxy = quaternion[k_XIndex] * quaternion[k_YIndex];
  T qyz = quaternion[k_YIndex] * quaternion[k_ZIndex];
  T qzx = quaternion[k_ZIndex] * quaternion[k_XIndex];

  T qxw = quaternion[k_XIndex] * quaternion[k_WIndex];
  T qyw = quaternion[k_YIndex] * quaternion[k_WIndex];
  T qzw = quaternion[k_ZIndex] * quaternion[k_WIndex];

  Eigen::Vector3<T> out = {};

  out[0] = multiplier[k_XIndex] * (squares[k_XIndex] - squares[k_YIndex] - squares[k_ZIndex] + squares[k_WIndex]) + 2 * (multiplier[k_YIndex] * (qxy + qzw) + multiplier[k_ZIndex] * (qzx - qyw));
  out[1] = multiplier[k_YIndex] * (squares[k_YIndex] - squares[k_XIndex] - squares[k_ZIndex] + squares[k_WIndex]) + 2 * (multiplier[k_ZIndex] * (qyz + qxw) + multiplier[k_XIndex] * (qxy - qzw));
  out[2] = multiplier[k_ZIndex] * (squares[k_ZIndex] - squares[k_XIndex] - squares[k_YIndex] + squares[k_WIndex]) + 2 * (multiplier[k_XIndex] * (qzx + qyw) + multiplier[k_YIndex] * (qyz - qxw));

  return out;
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
    int32 feature1 = 0;
    int32 feature2 = 0;
    Matrix3x3 orientationMatrix = {};
    float64 w = 0.0;
    uint32 phase1 = 0;
    uint32 phase2 = 0;

    float64 axisdiff111 = 0.0;
    float64 angdiff60 = 0.0;
    Eigen::Vector3d n = {0.0, 0.0, 0.0};
    float64 incoherence = 0.0;
    float64 n1 = 0.0;
    float64 n2 = 0.0;
    float64 n3 = 0.0;

    Eigen::Vector4d misq;
    Eigen::Vector4d sym_q;
    Eigen::Vector4d s1_misq;
    Eigen::Vector4d s2_misq;

    Eigen::Vector3d xstl_norm = {0.0, 0.0, 0.0};
    Eigen::Vector3d s_xstl_norm = {0.0, 0.0, 0.0};

    for(usize i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      feature1 = m_FaceLabels[2 * i];
      feature2 = m_FaceLabels[(2 * i) + 1];

      if(feature1 > 0 && feature2 > 0 && m_FeaturePhases[feature1] == m_FeaturePhases[feature2])
      {
        w = std::numeric_limits<float32>::max();
        const Eigen::Vector4d q1(m_AvgQuats[feature1 * 4], m_AvgQuats[(feature1 * 4) + 1], m_AvgQuats[(feature1 * 4) + 2], m_AvgQuats[(feature1 * 4) + 3]);
        Eigen::Vector4d q2(m_AvgQuats[feature2 * 4], m_AvgQuats[(feature2 * 4) + 1], m_AvgQuats[(feature2 * 4) + 2], m_AvgQuats[(feature2 * 4) + 3]);

        phase1 = m_CrystalStructures[m_FeaturePhases[feature1]];
        phase2 = m_CrystalStructures[m_FeaturePhases[feature2]];
        if(phase1 == phase2)
        {
          const int32 nsym = m_OrientationOps[phase1]->getNumSymOps();
          q2 = q2.conjugate();
          misq = q1.cwiseProduct(q2);
          orientationMatrix = Matrix3x3{OrientationTransformation::qu2om<Eigen::Vector4d, std::vector<float64>>(q1, QuatD::Order::VectorScalar).data()};

          xstl_norm = Eigen::Vector3d{m_FaceNormals[3 * i], m_FaceNormals[(3 * i) + 1], m_FaceNormals[(3 * i) + 2]}.transpose() * orientationMatrix;

          for(int32 j = 0; j < nsym; j++)
          {
            m_OrientationOps[phase1]->getQuatSymOp(j).copyInto(sym_q.data(), QuatD::Order::VectorScalar);

            // calculate crystal direction parallel to normal
            s1_misq = misq.cwiseProduct(sym_q);

            s_xstl_norm = MultiplyQuatByVector(sym_q, xstl_norm);

            for(int32 k = 0; k < nsym; k++)
            {
              // calculate the symmetric misorienation
              m_OrientationOps[phase1]->getQuatSymOp(k).copyInto(sym_q.data(), QuatD::Order::VectorScalar);
              sym_q = sym_q.conjugate();
              s2_misq = sym_q.cwiseProduct(s1_misq);

              OrientationTransformation::qu2ax<Eigen::Vector4d, OrientationD>(s2_misq).toAxisAngle(n1, n2, n3, w);

              w = w * 180.0f / nx::core::Constants::k_PiD;
              axisdiff111 = acos((std::abs(n1) * std::numbers::inv_sqrt3_v<float32>) + (std::abs(n2) * std::numbers::inv_sqrt3_v<float32>) + (std::abs(n3) * std::numbers::inv_sqrt3_v<float32>));
              angdiff60 = std::abs(w - 60.0f);
              if(axisdiff111 < m_AxisTol && angdiff60 < m_AngTol)
              {
                n[0] = n1;
                n[1] = n2;
                n[2] = n3;
                m_TwinBoundaries->setValue(i, true);

                float64 cosTheta = 1.0;

                const float64 norm1 = n.norm();
                const float64 norm2 = s_xstl_norm.norm();

                if(norm1 != 0 && norm2 != 0)
                {
                  cosTheta = n.dot(s_xstl_norm) / (norm1 * norm2);
                }

                incoherence = 180.0 * std::acos(cosTheta) / nx::core::Constants::k_PiD;
                if(incoherence > 90.0)
                {
                  incoherence = 180.0 - incoherence;
                }
                if(incoherence < m_TwinBoundaryIncoherence[i])
                {
                  m_TwinBoundaryIncoherence[i] = static_cast<float32>(incoherence);
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
    int32 feature1 = 0;
    int32 feature2 = 0;
    float64 w = 0.0;
    uint32 phase1 = 0;
    uint32 phase2 = 0;

    float64 axisdiff111 = 0.0;
    float64 angdiff60 = 0.0;
    Eigen::Vector3d n = {0.0, 0.0, 0.0};
    float64 n1 = 0.0;
    float64 n2 = 0.0;
    float64 n3 = 0.0;

    Eigen::Vector4d misq;
    Eigen::Vector4d sym_q;
    Eigen::Vector4d s1_misq;
    Eigen::Vector4d s2_misq;

    for(usize i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      feature1 = m_FaceLabels[2 * i];
      feature2 = m_FaceLabels[(2 * i) + 1];

      if(feature1 > 0 && feature2 > 0 && m_FeaturePhases[feature1] == m_FeaturePhases[feature2])
      {
        w = std::numeric_limits<float32>::max();
        const Eigen::Vector4d q1(m_AvgQuats[feature1 * 4], m_AvgQuats[(feature1 * 4) + 1], m_AvgQuats[(feature1 * 4) + 2], m_AvgQuats[(feature1 * 4) + 3]);
        Eigen::Vector4d q2(m_AvgQuats[feature2 * 4], m_AvgQuats[(feature2 * 4) + 1], m_AvgQuats[(feature2 * 4) + 2], m_AvgQuats[(feature2 * 4) + 3]);

        phase1 = m_CrystalStructures[m_FeaturePhases[feature1]];
        phase2 = m_CrystalStructures[m_FeaturePhases[feature2]];
        if(phase1 == phase2)
        {
          const int32 nsym = m_OrientationOps[phase1]->getNumSymOps();
          q2 = q2.conjugate();
          misq = q1.cwiseProduct(q2);

          for(int32 j = 0; j < nsym; j++)
          {
            m_OrientationOps[phase1]->getQuatSymOp(j).copyInto(sym_q.data(), QuatD::Order::VectorScalar);
            // calculate crystal direction parallel to normal
            s1_misq = misq.cwiseProduct(sym_q);

            for(int32 k = 0; k < nsym; k++)
            {
              // calculate the symmetric misorienation
              m_OrientationOps[phase1]->getQuatSymOp(k).copyInto(sym_q.data(), QuatD::Order::VectorScalar);
              sym_q = sym_q.conjugate();
              s2_misq = sym_q.cwiseProduct(s1_misq);

              OrientationTransformation::qu2ax<Eigen::Vector4d, OrientationD>(s2_misq).toAxisAngle(n1, n2, n3, w);

              w = w * 180.0f / nx::core::Constants::k_PiD;
              axisdiff111 = acos((std::abs(n1) * std::numbers::inv_sqrt3_v<float32>) + (std::abs(n2) * std::numbers::inv_sqrt3_v<float32>) + (std::abs(n3) * std::numbers::inv_sqrt3_v<float32>));
              angdiff60 = std::abs(w - 60.0f);
              if(axisdiff111 < m_AxisTol && angdiff60 < m_AngTol)
              {
                n[0] = n1;
                n[1] = n2;
                n[2] = n3;
                m_TwinBoundaries->setValue(i, true);
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
    dataAlg.execute(
        CalculateTwinBoundaryWithIncoherenceImpl(angtol, axistol, faceLabels, faceNormals, avgQuats, featurePhases, crystalStructures, twinBoundaries, twinBoundaryIncoherence, m_ShouldCancel));
  }
  else
  {
    dataAlg.execute(CalculateTwinBoundaryImpl(angtol, axistol, faceLabels, avgQuats, featurePhases, crystalStructures, twinBoundaries, m_ShouldCancel));
  }

  return {};
}
