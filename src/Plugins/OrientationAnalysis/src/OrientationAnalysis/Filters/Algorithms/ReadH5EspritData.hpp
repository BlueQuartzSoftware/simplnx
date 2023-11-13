#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/ReadH5Data.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ReadH5EspritDataInputValues
{
  bool DegreesToRadians;
};

/**
 * @class ReadH5EspritData
 * @brief This filter will read a single .h5 file into a new Image Geometry, allowing the immediate use of Filters on the data instead of having to generate the intermediate .h5ebsd file.
 */

class ORIENTATIONANALYSIS_EXPORT ReadH5EspritData : public ReadH5Data<H5EspritReader>
{
public:
  ReadH5EspritData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadH5DataInputValues* inputValues,
                   ReadH5EspritDataInputValues* espritInputValues);
  ~ReadH5EspritData() noexcept override;

  ReadH5EspritData(const ReadH5EspritData&) = delete;
  ReadH5EspritData(ReadH5EspritData&&) noexcept = delete;
  ReadH5EspritData& operator=(const ReadH5EspritData&) = delete;
  ReadH5EspritData& operator=(ReadH5EspritData&&) noexcept = delete;

  Result<> operator()();

  Result<> copyRawEbsdData(int index) override;

private:
  const ReadH5EspritDataInputValues* m_EspritInputValues = nullptr;
};

} // namespace complex
