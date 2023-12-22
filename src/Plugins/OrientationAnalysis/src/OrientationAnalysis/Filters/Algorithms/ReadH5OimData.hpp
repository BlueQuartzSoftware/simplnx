#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/ReadH5Data.hpp"

namespace nx::core
{
/**
 * @class ReadH5OimData
 * @brief This filter will read a single .h5 file into a new Image Geometry, allowing the immediate use of Filters on the data instead of having to generate the
 * intermediate .h5ebsd file.
 */

class ORIENTATIONANALYSIS_EXPORT ReadH5OimData : public ReadH5Data<H5OIMReader>
{
public:
  ReadH5OimData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues);
  ~ReadH5OimData() noexcept override;

  ReadH5OimData(const ReadH5OimData&) = delete;
  ReadH5OimData(ReadH5OimData&&) noexcept = delete;
  ReadH5OimData& operator=(const ReadH5OimData&) = delete;
  ReadH5OimData& operator=(ReadH5OimData&&) noexcept = delete;

  Result<> operator()();

  Result<> copyRawEbsdData(int index) override;
};

} // namespace nx::core
