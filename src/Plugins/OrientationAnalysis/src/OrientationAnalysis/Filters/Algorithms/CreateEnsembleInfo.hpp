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
  EnsembleInfoParameter::ValueType ensemble;
  DataPath cell_ensemble_attribute_matrix_name;
  std::string crystal_structures_array_name;
  std::string phase_types_array_name;
  std::string phase_names_array_name;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT CreateEnsembleInfo
{
public:
  CreateEnsembleInfo(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateEnsembleInfoInputValues* inputValues);
  ~CreateEnsembleInfo() noexcept = default;

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
