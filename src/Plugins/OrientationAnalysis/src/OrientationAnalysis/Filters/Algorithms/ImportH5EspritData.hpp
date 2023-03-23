#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/ImportH5Data.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ImportH5EspritDataInputValues
{
  bool CombineEulerAngles;
  bool DegreesToRadians;
};

/**
 * @class ImportH5EspritData
 * @brief This filter will read a single .h5 file into a new Image Geometry, allowing the immediate use of Filters on the data instead of having to generate the intermediate .h5ebsd file.
 */

class ORIENTATIONANALYSIS_EXPORT ImportH5EspritData : public ImportH5Data<H5EspritReader>
{
public:
  ImportH5EspritData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ImportH5DataInputValues* inputValues,
                     ImportH5EspritDataInputValues* espritInputValues);
  ~ImportH5EspritData() noexcept override;

  ImportH5EspritData(const ImportH5EspritData&) = delete;
  ImportH5EspritData(ImportH5EspritData&&) noexcept = delete;
  ImportH5EspritData& operator=(const ImportH5EspritData&) = delete;
  ImportH5EspritData& operator=(ImportH5EspritData&&) noexcept = delete;

  Result<> operator()();

  Result<> copyRawEbsdData(int index) override;

private:
  const ImportH5EspritDataInputValues* m_EspritInputValues = nullptr;
};

} // namespace complex
