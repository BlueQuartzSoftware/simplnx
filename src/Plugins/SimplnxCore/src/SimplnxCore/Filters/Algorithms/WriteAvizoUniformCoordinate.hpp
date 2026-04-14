#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/AvizoWriter.hpp"

namespace nx::core
{
/**
 * @class WriteAvizoUniformCoordinate
 * @brief Writes an Avizo Uniform Coordinate data file containing FeatureIds.
 *
 * @section ooc_summary OOC Optimization Summary
 * Same chunked copyIntoBuffer() approach as WriteAvizoRectilinearCoordinate.
 * The FeatureIds array is read in 64K-tuple chunks to support OOC DataStore backends.
 */
class SIMPLNXCORE_EXPORT WriteAvizoUniformCoordinate : public AvizoWriter
{
public:
  WriteAvizoUniformCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  ~WriteAvizoUniformCoordinate() noexcept override;

  WriteAvizoUniformCoordinate(const WriteAvizoUniformCoordinate&) = delete;
  WriteAvizoUniformCoordinate(WriteAvizoUniformCoordinate&&) noexcept = delete;
  WriteAvizoUniformCoordinate& operator=(const WriteAvizoUniformCoordinate&) = delete;
  WriteAvizoUniformCoordinate& operator=(WriteAvizoUniformCoordinate&&) noexcept = delete;

  /**
   * @brief Executes the writer: generates header and writes data.
   */
  Result<> operator()();

protected:
  /**
   * @brief Generates the Avizo uniform coordinate file header.
   * @param outputFile FILE pointer to the open output file.
   */
  Result<> generateHeader(FILE* outputFile) const override;

  /**
   * @brief Writes FeatureIds using OOC-safe chunked I/O.
   * @param outputFile FILE pointer to the open output file.
   */
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace nx::core
