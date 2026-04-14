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
 * @brief The ReadH5EbsdInputValues struct
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
 * @brief Algorithm that reads H5Ebsd-format EBSD data (TSL .ang or Oxford .ctf stored in HDF5).
 *
 * Supports both TSL (H5AngVolumeReader) and Oxford (H5CtfVolumeReader) manufacturers.
 * After data import, optionally applies recommended sample/Euler reference frame rotations.
 *
 * @section ooc_summary OOC Optimization Summary
 * The CopyData helper uses copyFromBuffer() for each selected array (single bulk write).
 * Euler angle interleaving uses chunked buffers with optional hex correction and
 * degree-to-radian conversion. Phase and crystal structure arrays are cached locally
 * via copyIntoBuffer() when needed for per-cell correction lookups.
 */
class ReadH5Ebsd
{
public:
  ReadH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5EbsdInputValues* inputValues);
  ~ReadH5Ebsd() noexcept;

  ReadH5Ebsd(const ReadH5Ebsd&) = delete;            // Copy Constructor Not Implemented
  ReadH5Ebsd(ReadH5Ebsd&&) = delete;                 // Move Constructor Not Implemented
  ReadH5Ebsd& operator=(const ReadH5Ebsd&) = delete; // Copy Assignment Not Implemented
  ReadH5Ebsd& operator=(ReadH5Ebsd&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadH5EbsdInputValues* m_InputValues = nullptr;
};

} // namespace nx::core
