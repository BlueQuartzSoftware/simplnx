#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/EnsembleInfoParameter.hpp"

namespace complex
{
// The lists here are IN ORDER of the enumerations from the EBSDLib. DO NOT CHANGE THE ORDER.
enum class PhaseType : uint32
{
  Primary = 0,        //!<
  Precipitate = 1,    //!<
  Transformation = 2, //!<
  Matrix = 3,         //!<
  Boundary = 4,       //!<
  Unknown = 999,      //!<
  Any = 4294967295U
};

struct ORIENTATIONANALYSIS_EXPORT CreateEnsembleInfoInputValues
{
  EnsembleInfoParameter::ValueType Ensemble;
  DataPath CellEnsembleAttributeMatrixName;
  std::string CrystalStructuresArrayName;
  std::string PhaseTypesArrayName;
  std::string PhaseNamesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
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

} // namespace complex
