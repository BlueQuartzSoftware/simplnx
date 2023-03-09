#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT EnsembleInfoReaderInputValues
{
  std::string InputFile;
  DataPath DataContainerName;
  DataPath CellEnsembleAttributeMatrixName;
  DataPath CrystalStructuresArrayName;
  DataPath PhaseTypesArrayName;
};

/**
 * @class EnsembleInfoReader
 * @brief This filter reads in information about the crystal structure and phase types of all the Features that are contained in a Cell based volume.
 */

class ORIENTATIONANALYSIS_EXPORT EnsembleInfoReader
{
public:
  EnsembleInfoReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EnsembleInfoReaderInputValues* inputValues);
  ~EnsembleInfoReader() noexcept;

  EnsembleInfoReader(const EnsembleInfoReader&) = delete;
  EnsembleInfoReader(EnsembleInfoReader&&) noexcept = delete;
  EnsembleInfoReader& operator=(const EnsembleInfoReader&) = delete;
  EnsembleInfoReader& operator=(EnsembleInfoReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  static inline const std::string k_EnsembleInfo = "EnsembleInfo";
  static inline const std::string k_NumberPhases = "Number_Phases";

protected:
  Result<> readFile();

private:
  DataStructure& m_DataStructure;
  const EnsembleInfoReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
