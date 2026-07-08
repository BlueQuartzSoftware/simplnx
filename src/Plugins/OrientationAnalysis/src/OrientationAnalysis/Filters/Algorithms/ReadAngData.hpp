#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <EbsdLib/IO/TSL/AngReader.h>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ReadAngDataInputValues
{
  std::filesystem::path InputFile;
  DataPath DataContainerName;
  std::string CellAttributeMatrixName;
  std::string CellEnsembleAttributeMatrixName;
};

/**
 * @class ReadAngData
 * @brief This filter will read a single .ang file into a new Image Geometry, allowing the immediate use of Filters on the data instead of having to generate the intermediate
 * .h5ebsd file.
 */
class ORIENTATIONANALYSIS_EXPORT ReadAngData
{
public:
  ReadAngData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadAngDataInputValues* inputValues);
  ~ReadAngData() noexcept;

  ReadAngData(const ReadAngData&) = delete;            // Copy Constructor Not Implemented
  ReadAngData(ReadAngData&&) = delete;                 // Move Constructor Not Implemented
  ReadAngData& operator=(const ReadAngData&) = delete; // Copy Assignment Not Implemented
  ReadAngData& operator=(ReadAngData&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadAngDataInputValues* m_InputValues = nullptr;

  /**
   * @brief Populates the Ensemble Attribute Matrix arrays (CrystalStructures, MaterialName,
   * LatticeConstants) from the phase sections parsed out of the .ang header. Every slot is
   * first initialized to the "Invalid Phase" defaults, then overwritten per parsed phase.
   * @param reader The AngReader that has already successfully read the input file.
   * @return Error result if no phases were parsed or a phase index falls outside the ensemble arrays.
   */
  Result<> loadMaterialInfo(ebsdlib::AngReader* reader) const;

  /**
   * @brief Copies the per-point data columns from the AngReader into the Cell Attribute Matrix
   * arrays: remaps phase values < 1 to 1, interleaves phi1/PHI/phi2 into the 3-component
   * EulerAngles array, and copies the remaining columns verbatim.
   * @param reader The AngReader that has already successfully read the input file.
   * @return Error result if the reader produced fewer scan points than the preflight-sized geometry
   * expects (which would otherwise read past the reader's buffers).
   */
  Result<> copyRawEbsdData(ebsdlib::AngReader* reader) const;
};

} // namespace nx::core
