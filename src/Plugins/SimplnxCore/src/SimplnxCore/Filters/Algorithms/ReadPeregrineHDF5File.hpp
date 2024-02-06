#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
struct SIMPLNXCORE_EXPORT ReadPeregrineHDF5FileInputValues
{
  fs::path inputFilePath;
  std::string segmentationResultsList;
  bool readCameraData;
  bool readPartIds;
  bool readSampleIds;
  bool readSubvolume;
  std::vector<uint64> subvolumeDims;
  DataPath sliceDataImageGeomPath;
  std::string sliceDataCellAttrMatName;
  std::string cameraDataArrayName;
  std::string partIdsArrayName;
  std::string sampleIdsArrayName;
  DataPath registeredDataImageGeomPath;
  std::string registeredDataCellAttrMatName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */
class SIMPLNXCORE_EXPORT ReadPeregrineHDF5File
{
public:
  ReadPeregrineHDF5File(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadPeregrineHDF5FileInputValues* inputValues);
  ~ReadPeregrineHDF5File() noexcept;

  ReadPeregrineHDF5File(const ReadPeregrineHDF5File&) = delete;
  ReadPeregrineHDF5File(ReadPeregrineHDF5File&&) noexcept = delete;
  ReadPeregrineHDF5File& operator=(const ReadPeregrineHDF5File&) = delete;
  ReadPeregrineHDF5File& operator=(ReadPeregrineHDF5File&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadPeregrineHDF5FileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
