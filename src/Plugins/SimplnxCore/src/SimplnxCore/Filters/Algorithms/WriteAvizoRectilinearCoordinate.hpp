#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/AvizoWriter.hpp"

namespace nx::core
{
/**
 * @class WriteAvizoRectilinearCoordinate
 * @brief Writes an Avizo Rectilinear Coordinate data file containing FeatureIds and axis coordinates.
 *
 * @section ooc_summary OOC Optimization Summary
 * The FeatureIds array is read in chunks of 64K tuples via copyIntoBuffer() and written
 * to the output file per chunk, replacing the original raw pointer fwrite that required
 * in-memory data. This works with both in-memory and OOC DataStore backends.
 */
class SIMPLNXCORE_EXPORT WriteAvizoRectilinearCoordinate : public AvizoWriter
{
public:
  WriteAvizoRectilinearCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  ~WriteAvizoRectilinearCoordinate() noexcept override;

  WriteAvizoRectilinearCoordinate(const WriteAvizoRectilinearCoordinate&) = delete;
  WriteAvizoRectilinearCoordinate(WriteAvizoRectilinearCoordinate&&) noexcept = delete;
  WriteAvizoRectilinearCoordinate& operator=(const WriteAvizoRectilinearCoordinate&) = delete;
  WriteAvizoRectilinearCoordinate& operator=(WriteAvizoRectilinearCoordinate&&) noexcept = delete;

  /**
   * @brief Executes the writer: generates header and writes data.
   */
  Result<> operator()();

protected:
  /**
   * @brief Generates the Avizo rectilinear coordinate file header.
   * @param outputFile FILE pointer to the open output file.
   */
  Result<> generateHeader(FILE* outputFile) const override;

  /**
   * @brief Writes FeatureIds and rectilinear coordinates using OOC-safe chunked I/O.
   * @param outputFile FILE pointer to the open output file.
   */
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace nx::core
