#include "ComputeBoundaryStrengths.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/Quaternion.hpp>
#include <cmath>

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

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeBoundaryStrengths::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeBoundaryStrengths::operator()()
{
  auto orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const auto& surfaceMeshFaceLabelsArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  const auto& avgQuatsArrayRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  const auto& featurePhasesArrayRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& crystalStructuresArrayRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const usize numCrystalStructures = crystalStructuresArrayRef.getNumberOfTuples();

  auto& mPrimes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceMeshmPrimesArrayName);
  auto& f1s = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceMeshF1sArrayName);
  auto& f1sPts = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceMeshF1sptsArrayName);
  auto& f7s = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SurfaceMeshF7sArrayName);

  const usize numTriangles = surfaceMeshFaceLabelsArrayRef.getNumberOfTuples();

  float32 mPrime1 = NAN;
  float32 mPrime2 = NAN;
  float32 f11 = NAN;
  float32 f12 = NAN;
  float32 f1Spt1 = NAN;
  float32 f1Spt2 = NAN;
  float32 f71 = NAN;
  float32 f72 = NAN;

  nx::core::Vec3<float64> ld = {m_InputValues->Loading[0], m_InputValues->Loading[1], m_InputValues->Loading[2]};
  ld = ld.normalize();

  bool emitLaueClassWarning = false;

  for(usize faceIdx = 0; faceIdx < numTriangles; faceIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const int32 feature1Idx = surfaceMeshFaceLabelsArrayRef[faceIdx * 2];
    const int32 feature2Idx = surfaceMeshFaceLabelsArrayRef[faceIdx * 2 + 1];
    if(feature1Idx > 0 && feature2Idx > 0)
    {
      ebsdlib::QuatD q1(avgQuatsArrayRef[feature1Idx * 4], avgQuatsArrayRef[feature1Idx * 4 + 1], avgQuatsArrayRef[feature1Idx * 4 + 2], avgQuatsArrayRef[feature1Idx * 4 + 3]);
      ebsdlib::QuatD q2(avgQuatsArrayRef[feature2Idx * 4], avgQuatsArrayRef[feature2Idx * 4 + 1], avgQuatsArrayRef[feature2Idx * 4 + 2], avgQuatsArrayRef[feature2Idx * 4 + 3]);

      const int32 feature1PhaseIdx = featurePhasesArrayRef[feature1Idx];
      const int32 feature2PhaseIdx = featurePhasesArrayRef[feature2Idx];
      if(feature1PhaseIdx < 0 || static_cast<usize>(feature1PhaseIdx) >= numCrystalStructures)
      {
        return MakeErrorResult(-94740,
                               fmt::format("Feature Phases array '{}' has value {} at feature index {} for face index {}, but Crystal Structures array '{}' has {} tuples. Valid Phase indices are in "
                                           "[0, {}).",
                                           m_InputValues->FeaturePhasesArrayPath.toString(), feature1PhaseIdx, feature1Idx, faceIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                           numCrystalStructures, numCrystalStructures));
      }
      if(feature2PhaseIdx < 0 || static_cast<usize>(feature2PhaseIdx) >= numCrystalStructures)
      {
        return MakeErrorResult(-94740,
                               fmt::format("Feature Phases array '{}' has value {} at feature index {} for face index {}, but Crystal Structures array '{}' has {} tuples. Valid Phase indices are in "
                                           "[0, {}).",
                                           m_InputValues->FeaturePhasesArrayPath.toString(), feature2PhaseIdx, feature2Idx, faceIdx, m_InputValues->CrystalStructuresArrayPath.toString(),
                                           numCrystalStructures, numCrystalStructures));
      }
      const uint32 currentLaueIndex = crystalStructuresArrayRef[feature1PhaseIdx];
      if(feature1PhaseIdx == feature2PhaseIdx && currentLaueIndex != ebsdlib::CrystalStructure::Cubic_High)
      {
        emitLaueClassWarning = true;
      }
      const uint32 neighborLaueIndex = crystalStructuresArrayRef[feature2PhaseIdx];
      if(currentLaueIndex == neighborLaueIndex && feature1PhaseIdx > 0)
      {
        if(currentLaueIndex >= orientationOps.size())
        {
          return MakeErrorResult(-94741,
                                 fmt::format("Crystal Structures array '{}' has value {} at Phase index {} for face index {}, but only {} Laue operations are available. Valid Laue indices are in "
                                             "[0, {}).",
                                             m_InputValues->CrystalStructuresArrayPath.toString(), currentLaueIndex, feature1PhaseIdx, faceIdx, orientationOps.size(), orientationOps.size()));
        }
        const ebsdlib::LaueOps::Pointer& laueClass = orientationOps[currentLaueIndex];
        mPrime1 = static_cast<float32>(laueClass->getmPrime(q1, q2, ld.data()));
        mPrime2 = static_cast<float32>(laueClass->getmPrime(q2, q1, ld.data()));
        f11 = static_cast<float32>(laueClass->getF1(q1, q2, ld.data(), true));
        f12 = static_cast<float32>(laueClass->getF1(q2, q1, ld.data(), true));
        f1Spt1 = static_cast<float32>(laueClass->getF1spt(q1, q2, ld.data(), true));
        f1Spt2 = static_cast<float32>(laueClass->getF1spt(q2, q1, ld.data(), true));
        f71 = static_cast<float32>(laueClass->getF7(q1, q2, ld.data(), true));
        f72 = static_cast<float32>(laueClass->getF7(q2, q1, ld.data(), true));
      }
      else
      {
        mPrime1 = 0.0f;
        f11 = 0.0f;
        f1Spt1 = 0.0f;
        f71 = 0.0f;
        mPrime2 = 0.0f;
        f12 = 0.0f;
        f1Spt2 = 0.0f;
        f72 = 0.0f;
      }
    }
    else
    {
      mPrime1 = 0.0f;
      f11 = 0.0f;
      f1Spt1 = 0.0f;
      f71 = 0.0f;
      mPrime2 = 0.0f;
      f12 = 0.0f;
      f1Spt2 = 0.0f;
      f72 = 0.0f;
    }

    mPrimes[2 * faceIdx] = mPrime1;
    mPrimes[2 * faceIdx + 1] = mPrime2;
    f1s[2 * faceIdx] = f11;
    f1s[2 * faceIdx + 1] = f12;
    f1sPts[2 * faceIdx] = f1Spt1;
    f1sPts[2 * faceIdx + 1] = f1Spt2;
    f7s[2 * faceIdx] = f71;
    f7s[2 * faceIdx + 1] = f72;
  }

  if(emitLaueClassWarning)
  {
    return MakeWarningVoidResult(-94739, fmt::format("Compute Boundary Strengths supports Cubic m-3m. A same-phase boundary uses another crystal structure. Check the Crystal Structures array '{}'.",
                                                     m_InputValues->CrystalStructuresArrayPath.toString()));
  }

  return {};
}
