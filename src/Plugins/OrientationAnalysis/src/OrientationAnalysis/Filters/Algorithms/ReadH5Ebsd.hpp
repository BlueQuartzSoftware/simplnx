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

namespace EbsdLib
{
namespace EnsembleData
{
inline const std::string CrystalStructures("CrystalStructures");
inline const std::string LatticeConstants("LatticeConstants");
inline const std::string MaterialName("MaterialName");
} // namespace EnsembleData
} // namespace EbsdLib

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
  int32 eulerRepresentation = EbsdLib::AngleRepresentation::Radians;
  std::vector<std::string> hdf5DataPaths = {};
  bool useRecommendedTransform = {true};
  DataPath dataContainerPath;
  DataPath cellAttributeMatrixPath;
  DataPath cellEnsembleMatrixPath;
};

/**
 * @brief The ReadH5Ebsd class
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
