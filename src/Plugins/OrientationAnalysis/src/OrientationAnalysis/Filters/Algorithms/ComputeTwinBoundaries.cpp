#include "ComputeTwinBoundaries.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

#include <Eigen/Dense>

#include <cmath>

using namespace nx::core;

namespace
{
/**
 * @brief The CalculateTwinBoundaryImpl class implements a threaded algorithm that determines whether a boundary is twin related and calculates
 * the respective incoherence. The calculations are performed on a surface mesh.
 */
class CalculateTwinBoundaryImpl
{
public:
  CalculateTwinBoundaryImpl(float32 angtol, float32 axistol, const Int32AbstractDataStore& faceLabels, const Float64AbstractDataStore& faceNormals, const Float32AbstractDataStore& avgQuats,
                            const Int32AbstractDataStore& featurePhases, const UInt32AbstractDataStore& crystalStructures, std::unique_ptr<MaskCompare>& twinBoundaries,
                            Float32AbstractDataStore& twinBoundaryIncoherence, bool findCoherence, const std::atomic_bool& shouldCancel)
  : m_AxisTol(axistol)
  , m_AngTol(angtol)
  , m_FaceLabels(faceLabels)
  , m_FaceNormals(faceNormals)
  , m_FeaturePhases(featurePhases)
  , m_AvgQuats(avgQuats)
  , m_TwinBoundaries(twinBoundaries)
  , m_TwinBoundaryIncoherence(twinBoundaryIncoherence)
  , m_CrystalStructures(crystalStructures)
  , m_FindCoherence(findCoherence)
  , m_ShouldCancel(shouldCancel)
  , m_OrientationOps(LaueOps::GetAllOrientationOps())
  {
  }

  ~CalculateTwinBoundaryImpl() = default;

  void generate(usize start, usize end) const
  {
    int32 feature1 = 0, feature2 = 0;
    float64 normal[3] = {0.0, 0.0, 0.0};
    float64 g1[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    float64 w = 0.0;
    uint32 phase1 = 0, phase2 = 0;

    float64 axisdiff111 = 0.0, angdiff60 = 0.0;
    nx::core::Point3Dd n = {0.0, 0.0, 0.0};
    float64 incoherence = 0.0;
    float64 n1 = 0.0, n2 = 0.0, n3 = 0.0;

    QuatD misq;
    QuatD sym_q;
    QuatD s1_misq;
    QuatD s2_misq;

    // QuatF* quats = reinterpret_cast<QuatF*>(m_Quats);

    nx::core::Point3Dd xstl_norm = {0.0, 0.0, 0.0};
    nx::core::Point3Dd s_xstl_norm = {0.0, 0.0, 0.0};

    for(usize i = start; i < end; i++)
    {
      feature1 = m_FaceLabels[2 * i];
      feature2 = m_FaceLabels[2 * i + 1];
      if(m_FindCoherence)
      {
        normal[0] = m_FaceNormals[3 * i];
        normal[1] = m_FaceNormals[3 * i + 1];
        normal[2] = m_FaceNormals[3 * i + 2];
      }
      if(feature1 > 0 && feature2 > 0 && m_FeaturePhases[feature1] == m_FeaturePhases[feature2])
      {
        w = std::numeric_limits<float32>::max();
        const QuatD q1(m_AvgQuats[feature1 * 4 + 0], m_AvgQuats[feature1 * 4 + 1], m_AvgQuats[feature1 * 4 + 2], m_AvgQuats[feature1 * 4 + 3]);
        QuatD q2(m_AvgQuats[feature2 * 4 + 0], m_AvgQuats[feature2 * 4 + 1], m_AvgQuats[feature2 * 4 + 2], m_AvgQuats[feature2 * 4 + 3]);

        phase1 = m_CrystalStructures[m_FeaturePhases[feature1]];
        phase2 = m_CrystalStructures[m_FeaturePhases[feature2]];
        if(phase1 == phase2)
        {
          int32 nsym = m_OrientationOps[phase1]->getNumSymOps();
          q2 = q2.conjugate();
          misq = q1 * q2;
          OrientationTransformation::qu2om<QuatD, OrientationD>(q1).toGMatrix(g1);

          if(m_FindCoherence)
          {
            // Todo:
            //  - Change to Eigen
            nx::core::MatrixMath::Multiply3x3with3x1(g1, normal, xstl_norm.data());
          }

          for(int32 j = 0; j < nsym; j++)
          {
            sym_q = m_OrientationOps[phase1]->getQuatSymOp(j);
            // calculate crystal direction parallel to normal
            s1_misq = misq * sym_q;

            if(m_FindCoherence)
            {
              s_xstl_norm = sym_q.multiplyByVector(xstl_norm.data());
            }

            for(int32 k = 0; k < nsym; k++)
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
                m_TwinBoundaries->setValue(i, true);
                if(m_FindCoherence)
                {
                  incoherence = 180.0 * std::acos(nx::core::GeometryMath::CosThetaBetweenVectors(n, s_xstl_norm)) / nx::core::Constants::k_PiD;
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
  const Int32AbstractDataStore& m_FaceLabels;
  const Float64AbstractDataStore& m_FaceNormals;
  const Float32AbstractDataStore& m_AvgQuats;
  const Int32AbstractDataStore& m_FeaturePhases;
  const UInt32AbstractDataStore& m_CrystalStructures;
  std::unique_ptr<MaskCompare>& m_TwinBoundaries;
  Float32AbstractDataStore& m_TwinBoundaryIncoherence;
  bool m_FindCoherence;
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
  const auto& faceNormals = m_DataStructure.getDataAs<Float64Array>(m_InputValues->FaceNormalsArrayPath)->getDataStoreRef();
  const auto& avgQuats = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgQuatsArrayPath)->getDataStoreRef();
  const auto& featurePhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath)->getDataStoreRef();
  const auto& crystalStructures = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath)->getDataStoreRef();

  std::unique_ptr<MaskCompare> twinBoundaries = nullptr;
  try
  {
    twinBoundaries = InstantiateMaskCompare(m_DataStructure, m_InputValues->TwinBoundariesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->TwinBoundariesArrayPath.toString());
    return MakeErrorResult(-54900, message);
  }

  auto& twinBoundaryIncoherence = m_DataStructure.getDataAs<Float32Array>(m_InputValues->TwinBoundaryIncoherenceArrayPath)->getDataStoreRef();

  float32 angtol = m_InputValues->AngleTolerance;
  float32 axistol = m_InputValues->AxisTolerance * Constants::k_PiF / 180.0f;

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, faceLabels.getNumberOfTuples());
  dataAlg.execute(CalculateTwinBoundaryImpl(angtol, axistol, faceLabels, faceNormals, avgQuats, featurePhases, crystalStructures, twinBoundaries, twinBoundaryIncoherence, m_InputValues->FindCoherence,
                                            m_ShouldCancel));

  return {};
}
