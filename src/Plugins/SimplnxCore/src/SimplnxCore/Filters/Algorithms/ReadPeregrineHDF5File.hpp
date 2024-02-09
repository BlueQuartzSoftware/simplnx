#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
struct SIMPLNXCORE_EXPORT ReadPeregrineHDF5FileInputValues
{
  fs::path inputFilePath;
  std::string segmentationResultsStr;
  bool readCameraData;
  bool readPartIds;
  bool readSampleIds;
  bool readSlicesSubvolume;
  std::vector<uint64> slicesSubvolumeMinMaxX;
  std::vector<uint64> slicesSubvolumeMinMaxY;
  std::vector<uint64> slicesSubvolumeMinMaxZ;
  DataPath sliceDataImageGeomPath;
  std::string sliceDataCellAttrMatName;
  std::string cameraData0ArrayName;
  std::string cameraData1ArrayName;
  std::string partIdsArrayName;
  std::string sampleIdsArrayName;
  DataPath registeredDataImageGeomPath;
  std::string registeredDataCellAttrMatName;
  bool readRegisteredDataSubvolume;
  std::vector<uint64> registeredDataSubvolumeMinMaxX;
  std::vector<uint64> registeredDataSubvolumeMinMaxY;
  std::vector<uint64> registeredDataSubvolumeMinMaxZ;
  bool readAnomalyDetection;
  std::string anomalyDetectionArrayName;
  bool readXRayCT;
  std::string xRayCTArrayName;
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

  static inline constexpr StringLiteral k_CameraDataPath0 = "/slices/camera_data/visible/0";
  static inline constexpr StringLiteral k_CameraDataPath1 = "/slices/camera_data/visible/1";
  static inline constexpr StringLiteral k_PartIdsPath = "/slices/part_ids";
  static inline constexpr StringLiteral k_SampleIdsPath = "/slices/sample_ids";
  static inline constexpr StringLiteral k_SegmentationResultsParentPath = "/slices/segmentation_results";
  static inline constexpr StringLiteral k_RegisteredAnomalyDetectionPath = "/slices/registered_data/anomaly_detection";
  static inline constexpr StringLiteral k_RegisteredXRayCTPath = "/slices/registered_data/x-ray_ct";

private:
  DataStructure& m_DataStructure;
  const ReadPeregrineHDF5FileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
