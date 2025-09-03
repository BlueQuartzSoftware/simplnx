#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT EMsoftSO3SamplerInputValues
{
  ChoicesParameter::ValueType sampleModeSelector;
  usize CrystalStructureIndex;
  bool OffsetGrid;
  float64 MisOr;
  VectorFloat32Parameter::ValueType RefOr;
  float64 MisOrFull;
  VectorFloat32Parameter::ValueType RefOrFull;
  int32 Numsp;
  DataPath EulerAnglesArrayName;
  DataPath EnsembleAttrMatrixPath;
};

namespace orientation_sampling
{
const std::string k_FZMode("Rodrigues fundamental zone (0)");
const std::string k_ConstantMode("Constant misorientation (1)");
const std::string k_LessThan("Less than given misorientation (2)");
const ChoicesParameter::Choices k_Choices = {k_FZMode, k_ConstantMode, k_LessThan};

constexpr ChoicesParameter::ValueType k_FZModeIndex = 0;
constexpr ChoicesParameter::ValueType k_ConstantModeIndex = 1;
constexpr ChoicesParameter::ValueType k_LessThanModeIndex = 2;
} // namespace orientation_sampling

/**
 * @class EMsoftSO3Sampler
 * @brief This algorithm implements support code for the EMsoftSO3SamplerFilter
 */

class ORIENTATIONANALYSIS_EXPORT EMsoftSO3Sampler
{
public:
  EMsoftSO3Sampler(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EMsoftSO3SamplerInputValues* inputValues);
  ~EMsoftSO3Sampler() noexcept;

  EMsoftSO3Sampler(const EMsoftSO3Sampler&) = delete;
  EMsoftSO3Sampler(EMsoftSO3Sampler&&) noexcept = delete;
  EMsoftSO3Sampler& operator=(const EMsoftSO3Sampler&) = delete;
  EMsoftSO3Sampler& operator=(EMsoftSO3Sampler&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const EMsoftSO3SamplerInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
