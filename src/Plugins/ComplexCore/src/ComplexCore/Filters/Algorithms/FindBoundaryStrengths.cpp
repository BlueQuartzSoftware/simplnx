#include "FindBoundaryStrengths.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

#include "EbsdLib/Core/Quaternion.hpp"

using namespace complex;

namespace
{

}

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
  size_t numTriangles = m_SurfaceMeshFaceLabelsPtr.lock()->getNumberOfTuples();

  double mPrime_1 = 0.0f, mPrime_2 = 0.0f, F1_1 = 0.0f, F1_2 = 0.0f, F1spt_1 = 0.0f, F1spt_2 = 0.0f, F7_1 = 0.0f, F7_2 = 0.0f;
  int32_t gname1 = 0, gname2 = 0;

  double LD[3] = {0.0f, 0.0f, 0.0f};

  LD[0] = m_Loading[0];
  LD[1] = m_Loading[1];
  LD[2] = m_Loading[2];
  MatrixMath::Normalize3x1(LD);

  for(size_t i = 0; i < numTriangles; i++)
  {
    gname1 = m_SurfaceMeshFaceLabels[i * 2];
    gname2 = m_SurfaceMeshFaceLabels[i * 2 + 1];
    if(gname1 > 0 && gname2 > 0)
    {
      QuatD q1(m_AvgQuats[gname1 * 4], m_AvgQuats[gname1 * 4 + 1], m_AvgQuats[gname1 * 4 + 2], m_AvgQuats[gname1 * 4 + 3]);
      QuatD q2(m_AvgQuats[gname2 * 4], m_AvgQuats[gname2 * 4 + 1], m_AvgQuats[gname2 * 4 + 2], m_AvgQuats[gname2 * 4 + 3]);

      if(m_CrystalStructures[m_FeaturePhases[gname1]] == m_CrystalStructures[m_FeaturePhases[gname2]] && m_FeaturePhases[gname1] > 0)
      {
        mPrime_1 = m_OrientationOps[m_CrystalStructures[m_FeaturePhases[gname1]]]->getmPrime(q1, q2, LD);
        mPrime_2 = m_OrientationOps[m_CrystalStructures[m_FeaturePhases[gname1]]]->getmPrime(q2, q1, LD);
        F1_1 = m_OrientationOps[m_CrystalStructures[m_FeaturePhases[gname1]]]->getF1(q1, q2, LD, true);
        F1_2 = m_OrientationOps[m_CrystalStructures[m_FeaturePhases[gname1]]]->getF1(q2, q1, LD, true);
        F1spt_1 = m_OrientationOps[m_CrystalStructures[m_FeaturePhases[gname1]]]->getF1spt(q1, q2, LD, true);
        F1spt_2 = m_OrientationOps[m_CrystalStructures[m_FeaturePhases[gname1]]]->getF1spt(q2, q1, LD, true);
        F7_1 = m_OrientationOps[m_CrystalStructures[m_FeaturePhases[gname1]]]->getF7(q1, q2, LD, true);
        F7_2 = m_OrientationOps[m_CrystalStructures[m_FeaturePhases[gname1]]]->getF7(q2, q1, LD, true);
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

    m_SurfaceMeshmPrimes[2 * i] = mPrime_1;
    m_SurfaceMeshmPrimes[2 * i + 1] = mPrime_2;
    m_SurfaceMeshF1s[2 * i] = F1_1;
    m_SurfaceMeshF1s[2 * i + 1] = F1_2;
    m_SurfaceMeshF1spts[2 * i] = F1spt_1;
    m_SurfaceMeshF1spts[2 * i + 1] = F1spt_2;
    m_SurfaceMeshF7s[2 * i] = F7_1;
    m_SurfaceMeshF7s[2 * i + 1] = F7_2;
  }

  return {};
}
