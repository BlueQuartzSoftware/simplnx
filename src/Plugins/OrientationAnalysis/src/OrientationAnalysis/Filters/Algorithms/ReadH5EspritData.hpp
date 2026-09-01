#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/IEbsdOemReader.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ReadH5EspritDataInputValues
 * @brief Selects whether imported Euler angles convert from degrees to radians.
 */
struct ORIENTATIONANALYSIS_EXPORT ReadH5EspritDataInputValues
{
  bool DegreesToRadians;
};

/**
 * @class ReadH5EspritData
 * @brief Imports Bruker Esprit HDF5 EBSD scans into an ImageGeom.
 *
 * IEbsdOemReader loads one scan and checks cancellation between scans. This
 * class interleaves Euler channels in 65,536-tuple pages. It writes scalar and
 * pattern channels with one scan-sized destination transfer.
 *
 * The destination transfers support out-of-core stores, but the current method
 * does not inspect their Result values. Cancellation returns success between
 * scans and preserves completed scan data.
 */
class ORIENTATIONANALYSIS_EXPORT ReadH5EspritData : public IEbsdOemReader<ebsdlib::H5EspritReader>
{
public:
  /**
   * @brief Initializes a Bruker Esprit scan importer.
   * @param dataStructure Provides destination arrays and geometry.
   * @param mesgHandler Receives status messages.
   * @param shouldCancel Signals cancellation between scans.
   * @param inputValues Identifies scans and destination paths.
   * @param espritInputValues Selects Euler-angle conversion.
   * @pre All arguments outlive this importer.
   */
  ReadH5EspritData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadH5DataInputValues* inputValues,
                   ReadH5EspritDataInputValues* espritInputValues);
  ~ReadH5EspritData() noexcept override;

  ReadH5EspritData(const ReadH5EspritData&) = delete;
  ReadH5EspritData(ReadH5EspritData&&) noexcept = delete;
  ReadH5EspritData& operator=(const ReadH5EspritData&) = delete;
  ReadH5EspritData& operator=(ReadH5EspritData&&) noexcept = delete;

  /**
   * @brief Imports each selected scan through IEbsdOemReader.
   * @return Reader or missing-pattern-data errors.
   */
  Result<> operator()();

  /**
   * @brief Copies one loaded scan into its volume destination range.
   * @param index Zero-based scan index.
   * @return Error when requested pattern data is absent.
   *
   * The current method does not inspect destination bulk-I/O Result values.
   */
  Result<> copyRawEbsdData(int index) override;

private:
  const ReadH5EspritDataInputValues* m_EspritInputValues = nullptr;
};

} // namespace nx::core
