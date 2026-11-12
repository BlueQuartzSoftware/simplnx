#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"
#include "OrientationAnalysis/utilities/EbsdReaderUtilities.hpp"

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
    if constexpr(std::is_same_v<T, ebsdlib::H5OIMReader> || std::is_same_v<T, ebsdlib::H5EspritReader>)
    {
      if(m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath) == nullptr)
      {
        return MakeErrorResult(EbsdReaderUtilities::k_OemDestinationMismatch,
                               fmt::format("File '{}' has no destination geometry '{}'. Apply preflight actions before import.", m_Reader->getFileName(), m_InputValues->ImageGeometryPath.toString()));
      }
    }
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

      m_MessageHandler.sendMessage({IFilter::Message::Type::Info, fmt::format("Importing Index {}", currentScanName)});

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
   * @return Header, dataset, destination, EbsdLib read, or missing-phase errors.
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

    if constexpr(std::is_same_v<T, ebsdlib::H5OIMReader> || std::is_same_v<T, ebsdlib::H5EspritReader>)
    {
      if(m_InputValues->SelectedScanNames.scanNames.empty())
      {
        return MakeErrorResult(EbsdReaderUtilities::k_OemDestinationMismatch, fmt::format("File '{}' has 0 selected scans. Select at least one scan and run preflight.", m_Reader->getFileName()));
      }
      // EbsdLib can reorder pattern buffers during readFile, so check metadata first.
      if(const int32 err = m_Reader->readHeaderOnly(); err < 0)
      {
        return MakeErrorResult(-8970, fmt::format("Could not read scan '{}' in file '{}': {}", scanName, m_Reader->getFileName(), m_Reader->getErrorMessage()));
      }
      constexpr int32 k_MissingPatternDimensions = std::is_same_v<T, ebsdlib::H5OIMReader> ? EbsdReaderUtilities::k_OimMissingPatternDimensions : EbsdReaderUtilities::k_EspritMissingPatternDimensions;
      if(auto result = EbsdReaderUtilities::ValidateOemHeader(*m_Reader, m_InputValues->ReadPatternData, k_MissingPatternDimensions); result.invalid())
      {
        return result;
      }
      auto first = T::New();
      first->setFileName(m_Reader->getFileName());
      first->setHDF5Path(m_InputValues->SelectedScanNames.scanNames.front());
      first->setReadPatternData(m_InputValues->ReadPatternData);
      if(const int32 err = first->readHeaderOnly(); err < 0)
      {
        return MakeErrorResult(-8970, fmt::format("Could not read scan '{}' in file '{}': {}", first->getHDF5Path(), first->getFileName(), first->getErrorMessage()));
      }
      if(auto result = EbsdReaderUtilities::ValidateOemScanCompatibility(*first, *m_Reader, m_InputValues->ReadPatternData); result.invalid())
      {
        return result;
      }
      if(m_InputValues->ReadPatternData)
      {
        if(auto result = EbsdReaderUtilities::ValidateOemPatternDataset(*m_Reader); result.invalid())
        {
          return result;
        }
      }
      const auto& geometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
      if(geometry.getNumXCells() != static_cast<usize>(m_Reader->getXDimension()) || geometry.getNumYCells() != static_cast<usize>(m_Reader->getYDimension()) ||
         geometry.getNumZCells() != m_InputValues->SelectedScanNames.scanNames.size())
      {
        return MakeErrorResult(EbsdReaderUtilities::k_OemDestinationMismatch,
                               fmt::format("File '{}' scan '{}' grid X={}, Y={}, selected scans={} does not match destination geometry '{}' X={}, Y={}, Z={}. Run preflight again.",
                                           m_Reader->getFileName(), scanName, m_Reader->getXDimension(), m_Reader->getYDimension(), m_InputValues->SelectedScanNames.scanNames.size(),
                                           m_InputValues->ImageGeometryPath.toString(), geometry.getNumXCells(), geometry.getNumYCells(), geometry.getNumZCells()));
      }
      if(m_InputValues->ReadPatternData)
      {
        std::array<int32, 2> dims = {};
        m_Reader->getPatternDims(dims);
        const std::string name = std::is_same_v<T, ebsdlib::H5EspritReader> ? ebsdlib::H5Esprit::RawPatterns : ebsdlib::Ang::PatternData;
        const auto path = m_InputValues->CellAttributeMatrixPath.createChildPath(name);
        const auto* patterns = m_DataStructure.getDataAs<UInt8Array>(path);
        if(patterns == nullptr || patterns->getComponentShape() != ShapeType{static_cast<usize>(dims[0]), static_cast<usize>(dims[1])})
        {
          return MakeErrorResult(EbsdReaderUtilities::k_OemPatternDestinationMismatch,
                                 fmt::format("File '{}' scan '{}' pattern height={}, width={} does not match destination '{}'. Run preflight again.", m_Reader->getFileName(), scanName, dims[0],
                                             dims[1], path.toString()));
        }
      }
    }

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

    if constexpr(std::is_same_v<T, ebsdlib::H5OIMReader> || std::is_same_v<T, ebsdlib::H5EspritReader>)
    {
      for(const auto& phase : phases)
      {
        const int32 id = phase == nullptr ? -1 : phase->getPhaseIndex();
        if(id <= 0 || static_cast<usize>(id) >= crystalStructures.getNumberOfTuples() || static_cast<usize>(id) >= materialNames.getNumberOfTuples() ||
           static_cast<usize>(id) >= latticeConstantsArray.getNumberOfTuples() || phase->getLatticeConstants().size() != 6)
        {
          return MakeErrorResult(
              EbsdReaderUtilities::k_OemPhaseDestinationMismatch,
              fmt::format("File '{}' scan '{}' phase ID {} does not fit ensemble '{}' with {} tuples or does not have six lattice constants. Run preflight again with a valid phase table.",
                          m_Reader->getFileName(), scanName, id, m_InputValues->CellEnsembleAttributeMatrixPath.toString(), crystalStructures.getNumberOfTuples()));
        }
      }
    }

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
