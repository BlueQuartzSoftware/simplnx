#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilePathGenerator.hpp"

namespace nx::core
{
namespace EbsdToH5EbsdInputConstants
{
const ChoicesParameter::Choices k_StackingChoices = {"Low to High", "High To Low"};
const ChoicesParameter::ValueType k_LowToHigh = static_cast<ChoicesParameter::ValueType>(nx::core::FilePathGenerator::Ordering::LowToHigh);
const ChoicesParameter::ValueType k_HighToLow = static_cast<ChoicesParameter::ValueType>(nx::core::FilePathGenerator::Ordering::HighToLow);

const ChoicesParameter::Choices k_TransformChoices = {"Edax - TSL", "Oxford - CTF", "No/Unknown Transformation", "HEDM - IceNine"};
const ChoicesParameter::ValueType k_Edax = 0;
const ChoicesParameter::ValueType k_Oxford = 1;
const ChoicesParameter::ValueType k_Unknown = 2;
const ChoicesParameter::ValueType k_Hedm = 3;

const size_t k_AngleIndex = 3;

const std::vector<float32> k_EdaxEulerTransform = {0.0F, 0.0F, 1.0F, 90.0F};
const std::vector<float32> k_EdaxSampleTransform = {0.0F, 1.0F, 0.0F, 180.0F};

const std::vector<float32> k_OxfordEulerTransform = {0.0F, 0.0F, 1.0F, 0.0F};
const std::vector<float32> k_OxfordSampleTransform = {0.0F, 1.0F, 0.0F, 180.0F};

const std::vector<float32> k_NoEulerTransform = {0.0F, 0.0F, 1.0F, 0.0F};
const std::vector<float32> k_NoSampleTransform = {0.0F, 0.0F, 1.0F, 0.0F};

const std::vector<float32> k_HedmEulerTransform = {0.0F, 0.0F, 1.0F, 0.0F};
const std::vector<float32> k_HedmSampleTransform = {0.0F, 0.0F, 1.0F, 0.0F};

} // namespace EbsdToH5EbsdInputConstants

struct ORIENTATIONANALYSIS_EXPORT EbsdToH5EbsdInputValues
{

  Float32Parameter::ValueType ZSpacing;
  ChoicesParameter::ValueType StackingOrder;
  ChoicesParameter::ValueType ReferenceFrame;
  FileSystemPathParameter::ValueType OutputPath;
  GeneratedFileListParameter::ValueType InputFileListInfo;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT EbsdToH5Ebsd
{
public:
  EbsdToH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EbsdToH5EbsdInputValues* inputValues);
  ~EbsdToH5Ebsd() noexcept;

  EbsdToH5Ebsd(const EbsdToH5Ebsd&) = delete;
  EbsdToH5Ebsd(EbsdToH5Ebsd&&) noexcept = delete;
  EbsdToH5Ebsd& operator=(const EbsdToH5Ebsd&) = delete;
  EbsdToH5Ebsd& operator=(EbsdToH5Ebsd&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const EbsdToH5EbsdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
