#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/IEbsdOemReader.hpp"

namespace nx::core
{
/**
 * @class ReadH5OimData
 * @brief Imports EDAX OIM HDF5 EBSD scans into an ImageGeom.
 *
 * EbsdLib owns one scan at a time. This importer converts and writes that scan
 * in pages of at most 65,536 tuples. It supports out-of-core destinations without
 * a second scan-sized staging allocation.
 *
 * Cancellation returns success and preserves completed destination pages. The
 * importer maps nonpositive source phase IDs to phase one.
 */

class ORIENTATIONANALYSIS_EXPORT ReadH5OimData : public IEbsdOemReader<ebsdlib::H5OIMReader>
{
public:
  /**
   * @brief Initializes an OIM scan importer.
   * @param dataStructure Provides destination arrays and geometry.
   * @param mesgHandler Receives status messages.
   * @param shouldCancel Signals cancellation between destination pages.
   * @param inputValues Identifies scans, options, and destination paths.
   * @pre All arguments outlive this importer.
   */
  ReadH5OimData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues);
  ~ReadH5OimData() noexcept override;

  ReadH5OimData(const ReadH5OimData&) = delete;
  ReadH5OimData(ReadH5OimData&&) noexcept = delete;
  ReadH5OimData& operator=(const ReadH5OimData&) = delete;
  ReadH5OimData& operator=(ReadH5OimData&&) noexcept = delete;

  /**
   * @brief Imports each selected scan through IEbsdOemReader.
   * @return Reader, destination transfer, or missing-pattern-data errors.
   */
  Result<> operator()();

  /**
   * @brief Converts and copies one loaded scan into its volume range.
   * @param sliceIndex Zero-based scan index.
   * @return Destination transfer or missing-pattern-data errors.
   */
  Result<> copyRawEbsdData(int sliceIndex) override;
};

} // namespace nx::core
