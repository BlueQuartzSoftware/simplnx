#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <EbsdLib/IO/HKL/CtfReader.h>

namespace nx::core
{

/**
 * @struct ReadCtfDataInputValues
 * @brief Identifies .ctf reader inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ReadCtfDataInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  DataPath DataContainerName;
  std::string CellAttributeMatrixName;
  std::string CellEnsembleAttributeMatrixName;
  bool DegreesToRadians;
  bool EdaxHexagonalAlignment;
};

/**
 * @class ReadCtfData
 * @brief Reads one .ctf EBSD file into an Image Geometry.
 *
 * EbsdLib owns full reader buffers. Simplnx interleaves Euler components in
 * bounded chunks with optional hex correction and unit conversion. Destination
 * bulk-write and crystal-structure bulk-read Result values are not inspected.
 */
class ORIENTATIONANALYSIS_EXPORT ReadCtfData
{
public:
  /**
   * @brief Initializes .ctf file reading.
   * @param dataStructure Provides output arrays.
   * @param msgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies input and output paths.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ReadCtfData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadCtfDataInputValues* inputValues);
  /**
   * @brief Destroys the .ctf reader.
   */
  ~ReadCtfData() noexcept;

  ReadCtfData(const ReadCtfData&) = delete;
  ReadCtfData(ReadCtfData&&) noexcept = delete;
  ReadCtfData& operator=(const ReadCtfData&) = delete;
  ReadCtfData& operator=(ReadCtfData&&) noexcept = delete;

  /**
   * @brief Reads the .ctf file into output arrays.
   * @return Success, or an EbsdLib or input-validation error.
   *
   * When a cancellation checkpoint observes the signal, the function returns success. Data copied before that checkpoint remains in the output arrays. Later data is not copied.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadCtfDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  /**
   * @brief Initializes ensemble arrays from parsed .ctf phases.
   * @param reader Provides parsed .ctf data.
   * @return An error if the file declares no phases.
   */
  Result<> loadMaterialInfo(ebsdlib::CtfReader* reader) const;

  /**
   * @brief Copies parsed .ctf cell columns to output arrays.
   * @param reader Provides parsed .ctf data.
   * @return An error if reader buffers are shorter than the output geometry.
   */
  Result<> copyRawEbsdData(ebsdlib::CtfReader* reader) const;
};

} // namespace nx::core
