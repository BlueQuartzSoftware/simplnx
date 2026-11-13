#include "ComputeBoundaryStrengths.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

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
Result<> ComputeBoundaryStrengths::ValidateLoadingDirection(const std::vector<float64>& loading)
{
  if(!OrientationUtilities::NormalizeDirection(loading[0], loading[1], loading[2]).has_value())
  {
    return MakeErrorResult(k_InvalidLoadingDirectionError,
                           fmt::format("The Loading Direction (XYZ) is [{}, {}, {}]. The direction needs finite components and a length greater than 0. Enter a direction such as [0, 0, 1].",
                                       loading[0], loading[1], loading[2]));
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> ComputeBoundaryStrengths::operator()()
{
  if(Result<> loadingResult = ValidateLoadingDirection(m_InputValues->Loading); loadingResult.invalid())
  {
    return loadingResult;
  }

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

  float32 mPrime_1, mPrime_2, F1_1, F1_2, F1spt_1, F1spt_2, F7_1, F7_2;

  // ValidateLoadingDirection() rejected a direction that cannot be normalized.
  const std::array<float64, 3> normalizedLoading = OrientationUtilities::NormalizeDirection(m_InputValues->Loading[0], m_InputValues->Loading[1], m_InputValues->Loading[2]).value();
  nx::core::Vec3<float64> LD = {normalizedLoading[0], normalizedLoading[1], normalizedLoading[2]};

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
      if(feature1PhaseIdx == feature2PhaseIdx && feature1PhaseIdx != 1)
      {
        emitLaueClassWarning = true;
      }
      const uint32 currentLaueIndex = crystalStructuresArrayRef[feature1PhaseIdx];
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
        ebsdlib::LaueOps::Pointer laueClass = orientationOps[currentLaueIndex];
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

    mPrimes[2 * faceIdx] = mPrime_1;
    mPrimes[2 * faceIdx + 1] = mPrime_2;
    f1s[2 * faceIdx] = F1_1;
    f1s[2 * faceIdx + 1] = F1_2;
    f1sPts[2 * faceIdx] = F1spt_1;
    f1sPts[2 * faceIdx + 1] = F1spt_2;
    f7s[2 * faceIdx] = F7_1;
    f7s[2 * faceIdx + 1] = F7_2;
  }

  if(emitLaueClassWarning)
  {
    return MakeWarningVoidResult(-94739, fmt::format("A phase other then Cubic m-3m is being analyzed. This filter only works on Cubic m-3m Laue classes. Those phases have a result of 0.0."));
  }

  return {};
}
