#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ReadEnsembleInfoInputValues
{
  std::string InputFile;
  DataPath DataContainerName;
  DataPath CellEnsembleAttributeMatrixName;
  DataPath CrystalStructuresArrayName;
  DataPath PhaseTypesArrayName;
};

/**
 * @class ReadEnsembleInfo
 * @brief This filter reads in information about the crystal structure and phase types of all the Features that are contained in a Cell based volume.
 */

class ORIENTATIONANALYSIS_EXPORT ReadEnsembleInfo
{
public:
  ReadEnsembleInfo(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadEnsembleInfoInputValues* inputValues);
  ~ReadEnsembleInfo() noexcept;

  ReadEnsembleInfo(const ReadEnsembleInfo&) = delete;
  ReadEnsembleInfo(ReadEnsembleInfo&&) noexcept = delete;
  ReadEnsembleInfo& operator=(const ReadEnsembleInfo&) = delete;
  ReadEnsembleInfo& operator=(ReadEnsembleInfo&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  static inline const std::string k_EnsembleInfo = "EnsembleInfo";
  static inline const std::string k_NumberPhases = "Number_Phases";

protected:
  Result<> readFile();

private:
  DataStructure& m_DataStructure;
  const ReadEnsembleInfoInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
