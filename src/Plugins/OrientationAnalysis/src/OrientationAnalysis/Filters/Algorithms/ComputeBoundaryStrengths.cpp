#include "ComputeBoundaryStrengths.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/Quaternion.hpp>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeBoundaryStrengths::ComputeBoundaryStrengths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   ComputeBoundaryStrengthsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundaryStrengths::~ComputeBoundaryStrengths() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeBoundaryStrengths::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeBoundaryStrengths::operator()()
{
  auto orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

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

  nx::core::Vec3<float64> LD = {m_InputValues->Loading[0], m_InputValues->Loading[1], m_InputValues->Loading[2]};
  LD = LD.normalize();

  bool emitLaueClassWarning = false;

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(numTriangles);
  progressHelper.setProgressMessageTemplate("Compute Boundary Strengths: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(usize i = 0; i < numTriangles; i++)
  {
    gName1 = surfaceMeshFaceLabels[i * 2];
    gName2 = surfaceMeshFaceLabels[i * 2 + 1];
    if(gName1 > 0 && gName2 > 0)
    {
      ebsdlib::QuatD q1(avgQuats[gName1 * 4], avgQuats[gName1 * 4 + 1], avgQuats[gName1 * 4 + 2], avgQuats[gName1 * 4 + 3]);
      ebsdlib::QuatD q2(avgQuats[gName2 * 4], avgQuats[gName2 * 4 + 1], avgQuats[gName2 * 4 + 2], avgQuats[gName2 * 4 + 3]);

      uint32 laueClassG1 = static_cast<uint32>(featurePhases[gName1]);
      uint32 laueClassG2 = static_cast<uint32>(featurePhases[gName2]);
      if(laueClassG1 == laueClassG2 && laueClassG1 != 1)
      {
        emitLaueClassWarning = true;
      }
      if(crystalStructures[laueClassG1] == crystalStructures[laueClassG2] && featurePhases[gName1] > 0)
      {
        ebsdlib::LaueOps::Pointer laueClass = orientationOps[crystalStructures[featurePhases[gName1]]];
        mPrime_1 = static_cast<float32>(laueClass->getmPrime(q1, q2, LD.data()));
        mPrime_2 = static_cast<float32>(laueClass->getmPrime(q2, q1, LD.data()));
        F1_1 = static_cast<float32>(laueClass->getF1(q1, q2, LD.data(), true));
        F1_2 = static_cast<float32>(laueClass->getF1(q2, q1, LD.data(), true));
        F1spt_1 = static_cast<float32>(laueClass->getF1spt(q1, q2, LD.data(), true));
        F1spt_2 = static_cast<float32>(laueClass->getF1spt(q2, q1, LD.data(), true));
        F7_1 = static_cast<float32>(laueClass->getF7(q1, q2, LD.data(), true));
        F7_2 = static_cast<float32>(laueClass->getF7(q2, q1, LD.data(), true));
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
    progressMessenger.sendProgressMessage(1);
  }

  if(emitLaueClassWarning)
  {
    return MakeWarningVoidResult(-94739, fmt::format("A phase other then Cubic m-3m is being analyzed. This filter only works on Cubic m-3m Laue classes. Those phases have a result of 0.0."));
  }

  return {};
}
