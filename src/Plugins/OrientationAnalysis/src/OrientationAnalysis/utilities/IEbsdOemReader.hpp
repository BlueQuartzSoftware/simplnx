#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <EbsdLib/IO/BrukerNano/H5EspritReader.h>
#include <EbsdLib/IO/HKL/H5OINAReader.h>
#include <EbsdLib/IO/TSL/H5OIMReader.h>

#include <fmt/format.h>

namespace nx::core
{
/**
 * @struct ReadH5DataInputValues
 * @brief Selects OEM scans, optional conversions, and destination paths.
 */
struct ORIENTATIONANALYSIS_EXPORT ReadH5DataInputValues
{
  OEMEbsdScanSelectionParameter::ValueType SelectedScanNames;
  bool ReadPatternData;
  DataPath ImageGeometryPath;
  DataPath CellAttributeMatrixPath;
  DataPath CellEnsembleAttributeMatrixPath;
  bool EdaxHexagonalAlignment;
  bool ConvertPhaseToInt32;
};

/**
 * @class IEbsdOemReader
 * @brief Provides the shared sequential workflow for OEM HDF5 EBSD readers.
 * @tparam T EbsdLib H5OIMReader, H5OINAReader, or H5EspritReader type.
 *
 * The base owns one EbsdLib reader and borrows the data structure, message
 * handler, cancellation flag, and input values. It loads one complete scan at
 * a time. Derived classes copy that scan into the volume arrays.
 *
 * Cancellation is checked before each scan. It returns success and preserves
 * scans copied before cancellation.
 */
template <class T>
class ORIENTATIONANALYSIS_EXPORT IEbsdOemReader
{
public:
  /**
   * @brief Initializes an OEM scan reader.
   * @param dataStructure Provides destination arrays and geometry.
   * @param mesgHandler Receives scan status messages.
   * @param shouldCancel Signals cancellation between scans.
   * @param inputValues Selects the file, scans, conversions, and destinations.
   * @pre All arguments outlive this reader.
   */
  IEbsdOemReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadH5DataInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(mesgHandler)
  , m_InputValues(inputValues)
  {
    m_Reader = T::New();
    m_Reader->setFileName(inputValues->SelectedScanNames.inputFilePath.string());
  }

  virtual ~IEbsdOemReader() noexcept = default;

  IEbsdOemReader(const IEbsdOemReader&) = delete;
  IEbsdOemReader(IEbsdOemReader&&) noexcept = delete;
  IEbsdOemReader& operator=(const IEbsdOemReader&) = delete;
  IEbsdOemReader& operator=(IEbsdOemReader&&) noexcept = delete;

  /**
   * @brief Loads and copies each selected scan in selection order.
   * @return Reader, phase-metadata, or derived copy errors.
   *
   * The method sets the destination geometry unit to micrometers before import.
   */
  Result<> execute()
  {
    auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
    imageGeom.setUnits(IGeometry::LengthUnit::Micrometer);

    const auto& scanNames = m_InputValues->SelectedScanNames.scanNames;
    int index = 0;
    for(const auto& currentScanName : scanNames)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Importing Index {}", currentScanName)});

      Result<> readResults = readData(currentScanName);
      if(readResults.invalid())
      {
        return readResults;
      }

      Result<> copyDataResults = copyRawEbsdData(index);
      if(copyDataResults.invalid())
      {
        return copyDataResults;
      }

      ++index;
    }
    return {};
  }

  const std::atomic_bool& getCancel()
  {
    return m_ShouldCancel;
  }

  /**
   * @brief Loads one scan and copies its phase metadata to ensemble arrays.
   * @param scanName HDF5 scan path selected by the user.
   * @return EbsdLib read or missing-phase errors.
   * @pre Ensemble arrays reserve index zero and contain every source phase index.
   * @pre Each source phase supplies six lattice constants.
   *
   * Standard AngFile ensemble names let all OEM formats feed the same downstream
   * orientation filters.
   */
  Result<> readData(const std::string& scanName)
  {
    m_Reader->setReadPatternData(m_InputValues->ReadPatternData);
    m_Reader->setHDF5Path(scanName);

    if(const int32 err = m_Reader->readFile(); err < 0)
    {
      return MakeErrorResult(-8970, fmt::format("Attempting to read scan '{}' from file '{}' produced an error from the '{}' class.\n  Error Code: {}\n  Message: {}", scanName,
                                                m_Reader->getFileName(), m_Reader->getNameOfClass(), m_Reader->getErrorCode(), m_Reader->getErrorMessage()));
    }

    const auto phases = m_Reader->getPhaseVector();
    if(phases.empty())
    {
      return MakeErrorResult(-8971, fmt::format("'{}' did not parse any phases from the .h5 file '{}' for scan '{}'", m_Reader->getNameOfClass(), scanName, m_Reader->getFileName()));
    }

    // Use shared AngFile ensemble names for every OEM reader format.
    auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::CrystalStructures));
    auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::MaterialName));
    auto& latticeConstantsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::LatticeConstants));
    Float32Array::store_type* latticeConstants = latticeConstantsArray.getDataStore();

    crystalStructures[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
    materialNames[0] = "Invalid Phase";
    latticeConstants->setComponent(0, 0, 0.0f);
    latticeConstants->setComponent(0, 1, 0.0f);
    latticeConstants->setComponent(0, 2, 0.0f);
    latticeConstants->setComponent(0, 3, 0.0f);
    latticeConstants->setComponent(0, 4, 0.0f);
    latticeConstants->setComponent(0, 5, 0.0f);
    for(const auto& phase : phases)
    {
      const int32 phaseId = phase->getPhaseIndex();
      crystalStructures[phaseId] = phase->determineOrientationOpsIndex();
      materialNames[phaseId] = phase->getMaterialName();
      std::vector<float32> lc = phase->getLatticeConstants();

      latticeConstants->setComponent(phaseId, 0, lc[0]);
      latticeConstants->setComponent(phaseId, 1, lc[1]);
      latticeConstants->setComponent(phaseId, 2, lc[2]);
      latticeConstants->setComponent(phaseId, 3, lc[3]);
      latticeConstants->setComponent(phaseId, 4, lc[4]);
      latticeConstants->setComponent(phaseId, 5, lc[5]);
    }

    return {};
  }

  /**
   * @brief Copies the currently loaded scan into its destination volume range.
   * @param index Zero-based scan index.
   * @return Source conversion or destination transfer errors.
   */
  virtual Result<> copyRawEbsdData(int index) = 0;

protected:
  std::shared_ptr<T> m_Reader;
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  const ReadH5DataInputValues* m_InputValues;
};

} // namespace nx::core
