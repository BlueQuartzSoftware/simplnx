#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/EnsembleInfoParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT CreateEnsembleInfoInputValues
{
  EnsembleInfoParameter::ValueType Ensemble;
  DataPath CellEnsembleAttributeMatrixName;
  std::string CrystalStructuresArrayName;
  std::string PhaseTypesArrayName;
  std::string PhaseNamesArrayName;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT CreateEnsembleInfo
{
public:
  CreateEnsembleInfo(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateEnsembleInfoInputValues* inputValues);
  ~CreateEnsembleInfo() noexcept;

  CreateEnsembleInfo(const CreateEnsembleInfo&) = delete;
  CreateEnsembleInfo(CreateEnsembleInfo&&) noexcept = delete;
  CreateEnsembleInfo& operator=(const CreateEnsembleInfo&) = delete;
  CreateEnsembleInfo& operator=(CreateEnsembleInfo&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateEnsembleInfoInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
