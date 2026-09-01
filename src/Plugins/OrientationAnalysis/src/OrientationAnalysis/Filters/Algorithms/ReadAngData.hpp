#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <EbsdLib/IO/TSL/AngReader.h>

namespace nx::core
{

/**
 * @struct ReadAngDataInputValues
 * @brief Identifies .ang reader inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ReadAngDataInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  DataPath DataContainerName;
  std::string CellAttributeMatrixName;
  std::string CellEnsembleAttributeMatrixName;
};

/**
 * @class ReadAngData
 * @brief Reads one .ang EBSD file into an Image Geometry.
 *
 * EbsdLib owns full reader buffers. Simplnx interleaves Euler components in
 * bounded chunks to avoid a second full-size staging buffer. Destination bulk-
 * write Result values are not inspected.
 */
class ORIENTATIONANALYSIS_EXPORT ReadAngData
{
public:
  /**
   * @brief Initializes .ang file reading.
   * @param dataStructure Provides output arrays.
   * @param msgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the input file and output paths.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ReadAngData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadAngDataInputValues* inputValues);
  /**
   * @brief Destroys the .ang reader.
   */
  ~ReadAngData() noexcept;

  ReadAngData(const ReadAngData&) = delete;
  ReadAngData(ReadAngData&&) = delete;
  ReadAngData& operator=(const ReadAngData&) = delete;
  ReadAngData& operator=(ReadAngData&&) = delete;

  /**
   * @brief Reads the .ang file into output arrays.
   * @return Success, or an EbsdLib or input-validation error.
   *
   * When a cancellation checkpoint observes the signal, the function returns success. Data copied before that checkpoint remains in the output arrays. Later data is not copied.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadAngDataInputValues* m_InputValues = nullptr;

  /**
   * @brief Initializes ensemble arrays from parsed .ang phases.
   * @param reader Provides parsed .ang data.
   * @return An error if phases are missing or exceed ensemble arrays.
   */
  Result<> loadMaterialInfo(ebsdlib::AngReader* reader) const;

  /**
   * @brief Copies parsed .ang cell columns to output arrays.
   * @param reader Provides parsed .ang data.
   * @return An error if reader buffers are shorter than the output geometry.
   */
  Result<> copyRawEbsdData(ebsdlib::AngReader* reader) const;
};

} // namespace nx::core
