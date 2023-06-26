#include "FindBoundaryStrengths.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"

#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

using namespace complex;

// -----------------------------------------------------------------------------
FindBoundaryStrengths::FindBoundaryStrengths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             FindBoundaryStrengthsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindBoundaryStrengths::~FindBoundaryStrengths() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindBoundaryStrengths::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindBoundaryStrengths::operator()()
{
  auto orientationOps = LaueOps::GetAllOrientationOps();

  auto& surfaceMeshFaceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  auto& mPrimes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceMeshmPrimesArrayName);
  auto& f1s = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceMeshF1sArrayName);
  auto& f1sPts = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceMeshF1sptsArrayName);
  auto& f7s = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceMeshF7sArrayName);

  usize numTriangles = surfaceMeshFaceLabels.getNumberOfTuples();

  float32 mPrime_1, mPrime_2, F1_1, F1_2, F1spt_1, F1spt_2, F7_1, F7_2;
  int32 gName1, gName2;

  float64 LD[3] = {m_InputValues->Loading[0], m_InputValues->Loading[1], m_InputValues->Loading[2]};
  MatrixMath::Normalize3x1(LD);

  for(usize i = 0; i < numTriangles; i++)
  {
    gName1 = surfaceMeshFaceLabels[i * 2];
    gName2 = surfaceMeshFaceLabels[i * 2 + 1];
    if(gName1 > 0 && gName2 > 0)
    {
      QuatD q1(avgQuats[gName1 * 4], avgQuats[gName1 * 4 + 1], avgQuats[gName1 * 4 + 2], avgQuats[gName1 * 4 + 3]);
      QuatD q2(avgQuats[gName2 * 4], avgQuats[gName2 * 4 + 1], avgQuats[gName2 * 4 + 2], avgQuats[gName2 * 4 + 3]);

      if(crystalStructures[featurePhases[gName1]] == crystalStructures[featurePhases[gName2]] && featurePhases[gName1] > 0)
      {
        mPrime_1 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[gName1]]]->getmPrime(q1, q2, LD));
        mPrime_2 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[gName1]]]->getmPrime(q2, q1, LD));
        F1_1 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[gName1]]]->getF1(q1, q2, LD, true));
        F1_2 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[gName1]]]->getF1(q2, q1, LD, true));
        F1spt_1 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[gName1]]]->getF1spt(q1, q2, LD, true));
        F1spt_2 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[gName1]]]->getF1spt(q2, q1, LD, true));
        F7_1 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[gName1]]]->getF7(q1, q2, LD, true));
        F7_2 = static_cast<float32>(orientationOps[crystalStructures[featurePhases[gName1]]]->getF7(q2, q1, LD, true));
      }
      else
      {
        mPrime_1 = 0.0f;
        F1_1 = 0.0f;
        F1spt_1 = 0.0f;
        F7_1 = 0.0f;
        mPrime_2 = 0.0f;
        F1_2 = 0.0f;
        F1spt_2 = 0.0f;
        F7_2 = 0.0f;
      }
    }
    else
    {
      mPrime_1 = 0.0f;
      F1_1 = 0.0f;
      F1spt_1 = 0.0f;
      F7_1 = 0.0f;
      mPrime_2 = 0.0f;
      F1_2 = 0.0f;
      F1spt_2 = 0.0f;
      F7_2 = 0.0f;
    }

    mPrimes[2 * i] = mPrime_1;
    mPrimes[2 * i + 1] = mPrime_2;
    f1s[2 * i] = F1_1;
    f1s[2 * i + 1] = F1_2;
    f1sPts[2 * i] = F1spt_1;
    f1sPts[2 * i + 1] = F1spt_2;
    f7s[2 * i] = F7_1;
    f7s[2 * i + 1] = F7_2;
  }

  return {};
}
