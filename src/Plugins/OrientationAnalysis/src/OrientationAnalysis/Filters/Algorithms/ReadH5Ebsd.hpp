#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/Parameters/ReadH5EbsdFileParameter.h"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <memory>
#include <mutex>
#include <vector>

class H5EbsdVolumeReader;
using H5EbsdVolumeReaderShrPtr = std::shared_ptr<H5EbsdVolumeReader>;

namespace ebsdlib
{
namespace EnsembleData
{
inline const std::string CrystalStructures("CrystalStructures");
inline const std::string LatticeConstants("LatticeConstants");
inline const std::string MaterialName("MaterialName");
} // namespace EnsembleData
} // namespace ebsdlib

namespace nx::core
{

/**
 * @struct ReadH5EbsdInputValues
 * @brief Defines the selected H5Ebsd volume, arrays, and destination paths.
 */
struct ORIENTATIONANALYSIS_EXPORT ReadH5EbsdInputValues
{
  std::string inputFilePath;
  int32 startSlice = 0;
  int32 endSlice = 0;
  int32 eulerRepresentation = ebsdlib::AngleRepresentation::Radians;
  std::vector<std::string> hdf5DataPaths = {};
  bool useRecommendedTransform = {true};
  DataPath dataContainerPath;
  DataPath cellAttributeMatrixPath;
  DataPath cellEnsembleMatrixPath;
};

/**
 * @class ReadH5Ebsd
 * @brief Imports TSL or Oxford EBSD volume data from an H5Ebsd file.
 *
 * The importer accepts H5AngVolumeReader and H5CtfVolumeReader files. It can
 * apply the stored Euler and sample reference-frame transforms after import.
 *
 * Destination writes use bulk DataStore operations and support out-of-core
 * stores. EbsdLib still materializes each selected source array. Oxford hexagonal
 * correction also caches the complete cell-phase array, so the import is not a
 * bounded-memory operation.
 *
 * The current import and conversion loops do not inspect cancellation. The flag
 * is passed only to optional downstream rotation filters. Several bulk-I/O
 * Result values are also not inspected, so operator() does not report those
 * transfer failures.
 */
class ReadH5Ebsd
{
public:
  /**
   * @brief Initializes an H5Ebsd volume importer.
   * @param dataStructure Provides destination arrays and geometry.
   * @param mesgHandler Receives status messages.
   * @param shouldCancel Supplies cancellation to optional rotation filters.
   * @param inputValues Identifies source slices, selected arrays, and destinations.
   * @pre All arguments outlive this importer.
   */
  ReadH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5EbsdInputValues* inputValues);
  ~ReadH5Ebsd() noexcept;

  ReadH5Ebsd(const ReadH5Ebsd&) = delete;
  ReadH5Ebsd(ReadH5Ebsd&&) = delete;
  ReadH5Ebsd& operator=(const ReadH5Ebsd&) = delete;
  ReadH5Ebsd& operator=(ReadH5Ebsd&&) = delete;

  /**
   * @brief Imports selected volume data and applies requested transforms.
   * @return File, manufacturer, EbsdLib, or rotation-filter errors.
   * @pre Slice bounds and selected destination arrays match the source volume.
   * @pre Cell phase IDs index the imported crystal-structure array.
   * @pre Application::Instance() exists when a sample transform is requested.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadH5EbsdInputValues* m_InputValues = nullptr;
};

} // namespace nx::core
