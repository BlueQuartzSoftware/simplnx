#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

#include "EbsdLib/IO/BrukerNano/H5EspritReader.h"
#include "EbsdLib/IO/TSL/H5OIMReader.h"

namespace complex
{
struct ORIENTATIONANALYSIS_EXPORT ImportH5DataInputValues
{
  OEMEbsdScanSelectionParameter::ValueType SelectedScanNames;
  bool ReadPatternData;
  DataPath ImageGeometryPath;
  DataPath CellAttributeMatrixPath;
  DataPath CellEnsembleAttributeMatrixPath;
};

/**
 * @class ImportH5Data
 */

template <class T>
class ORIENTATIONANALYSIS_EXPORT ImportH5Data
{
public:
  ImportH5Data(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ImportH5DataInputValues* inputValues);
  virtual ~ImportH5Data() noexcept;

  ImportH5Data(const ImportH5Data&) = delete;
  ImportH5Data(ImportH5Data&&) noexcept = delete;
  ImportH5Data& operator=(const ImportH5Data&) = delete;
  ImportH5Data& operator=(ImportH5Data&&) noexcept = delete;

  Result<> execute();

  const std::atomic_bool& getCancel();

  Result<> readData(const std::string& scanName);

  virtual Result<> copyRawEbsdData(int index) = 0;

protected:
  std::shared_ptr<T> m_Reader;
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  const ImportH5DataInputValues* m_InputValues;
};

template class ImportH5Data<H5OIMReader>;
template class ImportH5Data<H5EspritReader>;

} // namespace complex
