#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/IEbsdOemReader.hpp"

#include <string>

namespace nx::core
{

/**
 * @class ReadH5OinaData
 * @brief Reads one or more scans from an H5OINA file into one Image Geometry.
 */

class ORIENTATIONANALYSIS_EXPORT ReadH5OinaData : public IEbsdOemReader<ebsdlib::H5OINAReader>
{
public:
  ReadH5OinaData(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues);
  ~ReadH5OinaData() noexcept override;

  ReadH5OinaData(const ReadH5OinaData&) = delete;
  ReadH5OinaData(ReadH5OinaData&&) noexcept = delete;
  ReadH5OinaData& operator=(const ReadH5OinaData&) = delete;
  ReadH5OinaData& operator=(ReadH5OinaData&&) noexcept = delete;

  /**
   * @brief Imports each selected scan through IEbsdOemReader.
   * @return Reader, destination transfer, or missing-pattern-data errors.
   */
  Result<> operator()();

  Result<> copyRawEbsdData(int scanIndex) override;

private:
  /**
   * @brief The scan name whose data is currently in the reader's buffers. The shared
   * copyRawEbsdData(int) signature carries only the destination slab index, and under a
   * High-to-Low stacking order that index no longer matches the scan's position in the
   * selection list, so the name is carried here instead of being re-derived from it.
   */
  std::string m_CurrentScanName;
};

} // namespace nx::core
