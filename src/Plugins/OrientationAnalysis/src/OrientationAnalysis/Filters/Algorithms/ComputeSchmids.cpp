#include "ComputeSchmids.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeSchmids::ComputeSchmids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeSchmidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeSchmids::~ComputeSchmids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeSchmids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeSchmids::operator()()
{
  std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  const auto& avgQuatPtr = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  const auto& featurePhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto& schmidArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->SchmidsArrayName);
  auto& slipSystems = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SlipSystemsArrayName);
  auto& poleArrays = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->PolesArrayName);

  auto* phiArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->PhisArrayName);
  auto* lambdaArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->LambdasArrayName);
  if(m_InputValues->StoreAngleComponents)
  {
    (*phiArray)[0] = 0.0F;
    (*lambdaArray)[0] = 0.0F;
  }
  schmidArray[0] = 0.0F;
  poleArrays[0] = 0;
  poleArrays[1] = 0;
  poleArrays[2] = 0;
  slipSystems[0] = 0;

  size_t totalFeatures = avgQuatPtr.getNumberOfTuples();

  int32_t slipSystem = 0;

  Eigen::Vector3d sampleLoading = {m_InputValues->LoadingDirection[0], m_InputValues->LoadingDirection[1], m_InputValues->LoadingDirection[2]};
  sampleLoading.normalize();

  double angleComps[2] = {0.0f, 0.0f};
  double schmid = 0.0f;

  Eigen::Vector3d plane;
  Eigen::Vector3d direction;
  if(m_InputValues->OverrideSystem)
  {
    plane = {m_InputValues->SlipPlane[0], m_InputValues->SlipPlane[1], m_InputValues->SlipPlane[2]};
    plane.normalize();

    direction = {m_InputValues->SlipDirection[0], m_InputValues->SlipDirection[1], m_InputValues->SlipDirection[2]};
    direction.normalize();
  }

  for(size_t i = 1; i < totalFeatures; i++)
  {
    uint32_t laueClass = crystalStructures[featurePhases[i]];
    if(laueClass >= EbsdLib::CrystalStructure::LaueGroupEnd)
    {
      continue;
    }
    auto om = OrientationTransformation::qu2om<QuatF, OrientationD>({avgQuatPtr[i * 4 + 0], avgQuatPtr[i * 4 + 1], avgQuatPtr[i * 4 + 2], avgQuatPtr[i * 4 + 3]});
    auto g = OrientationUtilities::OrientationMatrixToGMatrix(om);
    Eigen::Vector3d crystalLoading = g * sampleLoading;

    if(!m_InputValues->OverrideSystem)
    {
      orientationOps[laueClass]->getSchmidFactorAndSS(crystalLoading.data(), schmid, angleComps, slipSystem);
    }
    else
    {
      orientationOps[laueClass]->getSchmidFactorAndSS(crystalLoading.data(), plane.data(), direction.data(), schmid, angleComps, slipSystem);
    }

    schmidArray[i] = static_cast<float>(schmid);
    if(m_InputValues->StoreAngleComponents)
    {
      (*phiArray)[i] = angleComps[0];
      (*lambdaArray)[i] = angleComps[1];
    }

    poleArrays[3 * i] = static_cast<int32>(crystalLoading[0] * 100.0);
    poleArrays[3 * i + 1] = static_cast<int32>(crystalLoading[1] * 100.0);
    poleArrays[3 * i + 2] = static_cast<int32>(crystalLoading[2] * 100.0);
    slipSystems[i] = slipSystem;
  }

  return {};
}
