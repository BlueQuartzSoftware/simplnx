#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/ImportH5Data.hpp"

namespace complex
{
/**
 * @class ImportH5OimData
 * @brief This filter will read a single .h5 file into a new Image Geometry, allowing the immediate use of Filters on the data instead of having to generate the
 * intermediate .h5ebsd file.
 */

class ORIENTATIONANALYSIS_EXPORT ImportH5OimData : public ImportH5Data<H5OIMReader>
{
public:
  ImportH5OimData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportH5DataInputValues* inputValues);
  ~ImportH5OimData() noexcept override;

  ImportH5OimData(const ImportH5OimData&) = delete;
  ImportH5OimData(ImportH5OimData&&) noexcept = delete;
  ImportH5OimData& operator=(const ImportH5OimData&) = delete;
  ImportH5OimData& operator=(ImportH5OimData&&) noexcept = delete;

  Result<> operator()();

  Result<> copyRawEbsdData(int index) override;
};

} // namespace complex
