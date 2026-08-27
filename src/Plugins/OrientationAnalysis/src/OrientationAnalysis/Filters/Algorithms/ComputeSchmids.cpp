#include "ComputeSchmids.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeSchmids::ComputeSchmids(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ComputeSchmidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSchmids::~ComputeSchmids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeSchmids::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeSchmids::operator()()
{
  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const auto& avgQuatsRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  const auto& featurePhasesRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& crystalStructuresRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto& schmidsRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SchmidsArrayName);
  auto& slipSystemsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SlipSystemsArrayName);
  auto& polesRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->PolesArrayName);

  auto* phisPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->PhisArrayName);
  auto* lambdasPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->LambdasArrayName);
  // Feature 0 is the conventional unassigned-Feature sentinel and is never computed; the loop below
  // starts at 1. Preflight now creates all five arrays with a "0" fill value so every tuple is
  // defined regardless, but these explicit writes are kept because they are what documents the
  // sentinel row's meaning at the point of use.
  if(m_InputValues->StoreAngleComponents)
  {
    (*phisPtr)[0] = 0.0F;
    (*lambdasPtr)[0] = 0.0F;
  }
  schmidsRef[0] = 0.0F;
  polesRef[0] = 0;
  polesRef[1] = 0;
  polesRef[2] = 0;
  slipSystemsRef[0] = 0;

  const usize featureCount = avgQuatsRef.getNumberOfTuples();
  const usize ensembleCount = crystalStructuresRef.getNumberOfTuples();

  Eigen::Vector3d sampleLoading = {m_InputValues->LoadingDirection[0], m_InputValues->LoadingDirection[1], m_InputValues->LoadingDirection[2]};
  sampleLoading.normalize();

  Eigen::Vector3d plane;
  Eigen::Vector3d direction;
  if(m_InputValues->OverrideSystem)
  {
    plane = {m_InputValues->SlipPlane[0], m_InputValues->SlipPlane[1], m_InputValues->SlipPlane[2]};
    plane.normalize();

    direction = {m_InputValues->SlipDirection[0], m_InputValues->SlipDirection[1], m_InputValues->SlipDirection[2]};
    direction.normalize();
  }

  for(usize featureIdx = 1; featureIdx < featureCount; featureIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    // The phase id indexes the Crystal Structures array directly, so it has to be validated before
    // it is used. An id at or beyond the ensemble count, or a negative one, would read outside the
    // array and dispatch on whatever Laue value that read produced.
    const int32 phaseId = featurePhasesRef[featureIdx];
    if(phaseId < 0)
    {
      return MakeErrorResult(-13502, fmt::format("Feature {} has a negative phase value of {}. Phase values index the Crystal Structures array '{}' and must be zero or greater.", featureIdx, phaseId,
                                                 m_InputValues->CrystalStructuresArrayPath.toString()));
    }
    if(static_cast<usize>(phaseId) >= ensembleCount)
    {
      return MakeErrorResult(-13501, fmt::format("Feature {} has a phase value of {} but the Crystal Structures array '{}' only has {} entries (valid phase values are 0 through {}).", featureIdx,
                                                 phaseId, m_InputValues->CrystalStructuresArrayPath.toString(), ensembleCount, ensembleCount - 1));
    }

    const uint32_t laueClass = crystalStructuresRef[phaseId];
    if(laueClass >= ebsdlib::CrystalStructure::LaueGroupEnd)
    {
      continue;
    }

    // Re-initialized every iteration on purpose. EbsdLib Laue ops that enumerate no slip systems
    // for their class return without writing every output, so a buffer hoisted out of the loop
    // would carry the previous Feature's values into the current Feature's row. Fixed in EbsdLib
    // 3.1.1 as well; keeping the locals loop-scoped means this filter is correct against
    // any EbsdLib.
    double schmid = 0.0;
    double angleComponents[2] = {0.0, 0.0};
    int32_t slipSystem = 0;

    const auto orientationMatrix =
        ebsdlib::QuaternionDType(avgQuatsRef[featureIdx * 4 + 0], avgQuatsRef[featureIdx * 4 + 1], avgQuatsRef[featureIdx * 4 + 2], avgQuatsRef[featureIdx * 4 + 3]).toOrientationMatrix();
    Eigen::Vector3d crystalLoading = orientationMatrix * sampleLoading;

    if(!m_InputValues->OverrideSystem)
    {
      orientationOps[laueClass]->getSchmidFactorAndSS(crystalLoading.data(), schmid, angleComponents, slipSystem);
    }
    else
    {
      orientationOps[laueClass]->getSchmidFactorAndSS(crystalLoading.data(), plane.data(), direction.data(), schmid, angleComponents, slipSystem);
    }

    schmidsRef[featureIdx] = static_cast<float>(schmid);
    if(m_InputValues->StoreAngleComponents)
    {
      (*phisPtr)[featureIdx] = angleComponents[0];
      (*lambdasPtr)[featureIdx] = angleComponents[1];
    }

    polesRef[3 * featureIdx] = static_cast<int32>(crystalLoading[0] * 100.0);
    polesRef[3 * featureIdx + 1] = static_cast<int32>(crystalLoading[1] * 100.0);
    polesRef[3 * featureIdx + 2] = static_cast<int32>(crystalLoading[2] * 100.0);
    slipSystemsRef[featureIdx] = slipSystem;
  }

  return {};
}
