#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/IEbsdOemReader.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ReadH5EspritDataInputValues
{
  bool DegreesToRadians;
};

/**
 * @class ReadH5EspritData
 * @brief Algorithm that reads Bruker Esprit H5OINA EBSD data into an Image Geometry.
 *
 * Reads one slice at a time from the HDF5 file via the EbsdLib H5EspritReader,
 * then copies each slice's data into the volume-wide DataStructure arrays.
 *
 * @section ooc_summary OOC Optimization Summary
 * Same copyFromBuffer() bulk write approach as the other EBSD readers. Euler angles use
 * chunked interleaving with optional degree-to-radian conversion. Single-component arrays
 * are written in one bulk call per slice at the correct offset.
 */
class ORIENTATIONANALYSIS_EXPORT ReadH5EspritData : public IEbsdOemReader<ebsdlib::H5EspritReader>
{
public:
  ReadH5EspritData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadH5DataInputValues* inputValues,
                   ReadH5EspritDataInputValues* espritInputValues);
  ~ReadH5EspritData() noexcept override;

  ReadH5EspritData(const ReadH5EspritData&) = delete;
  ReadH5EspritData(ReadH5EspritData&&) noexcept = delete;
  ReadH5EspritData& operator=(const ReadH5EspritData&) = delete;
  ReadH5EspritData& operator=(ReadH5EspritData&&) noexcept = delete;

  /**
   * @brief Executes the algorithm by delegating to the base class execute().
   */
  Result<> operator()();

  /**
   * @brief Copies raw EBSD data for one slice from the H5Esprit reader into the DataStructure.
   * @param index Zero-based slice index, used to compute the tuple offset.
   * @return Result<> indicating success or an error if pattern data is missing.
   */
  Result<> copyRawEbsdData(int index) override;

private:
  const ReadH5EspritDataInputValues* m_EspritInputValues = nullptr;
};

} // namespace nx::core
