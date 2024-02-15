#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/ReadH5Data.hpp"

namespace nx::core
{

/**
 * @class ReadH5OinaData
 * @brief This filter will read a single .h5 file into a new Image Geometry, allowing the immediate use of Filters on the data instead of having to generate the
 * intermediate .h5ebsd file.
 */

class ORIENTATIONANALYSIS_EXPORT ReadH5OinaData : public ReadH5Data<H5OINAReader>
{
public:
  ReadH5OinaData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues);
  ~ReadH5OinaData() noexcept override;

  ReadH5OinaData(const ReadH5OinaData&) = delete;
  ReadH5OinaData(ReadH5OinaData&&) noexcept = delete;
  ReadH5OinaData& operator=(const ReadH5OinaData&) = delete;
  ReadH5OinaData& operator=(ReadH5OinaData&&) noexcept = delete;

  Result<> operator()();

  Result<> copyRawEbsdData(int index) override;
};

} // namespace nx::core
