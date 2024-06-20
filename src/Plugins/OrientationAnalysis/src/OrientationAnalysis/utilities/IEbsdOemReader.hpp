#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "EbsdLib/IO/BrukerNano/H5EspritReader.h"
#include "EbsdLib/IO/HKL/H5OINAReader.h"
#include "EbsdLib/IO/TSL/H5OIMReader.h"

#include <fmt/format.h>

namespace nx::core
{
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
 * @class ReadH5Data
 */

template <class T>
class ORIENTATIONANALYSIS_EXPORT IEbsdOemReader
{
public:
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

  Result<> execute()
  {
    auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
    imageGeom.setUnits(IGeometry::LengthUnit::Micrometer);

    int index = 0;
    for(const auto& currentScanName : m_InputValues->SelectedScanNames.scanNames)
    {
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

  Result<> readData(const std::string& scanName)
  {
    m_Reader->setReadPatternData(m_InputValues->ReadPatternData);
    // If the user has already set a Scan Name to read then we are good to go.
    m_Reader->setHDF5Path(scanName);

    if(const int32 err = m_Reader->readFile(); err < 0)
    {
      return MakeErrorResult(-8970, fmt::format("Attempting to read scan '{}' from file '{}' produced an error from the '{}' class.\n  Error Code: {}\n  Message: {}", scanName,
                                                m_Reader->getFileName(), m_Reader->getNameOfClass(), m_Reader->getErrorCode(), m_Reader->getErrorMessage()));
    }

    const auto phases = m_Reader->getPhaseVector();
    if(phases.empty())
    {
      return MakeErrorResult(-8971, fmt::format("'{}' did not parse any phases from from the .h5 file '{}' for scan '{}'", m_Reader->getNameOfClass(), scanName, m_Reader->getFileName()));
    }

    // These arrays are purposely created using the AngFile constant names for BOTH the Oim and the Esprit readers!
    auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::CrystalStructures));
    auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::MaterialName));
    auto& latticeConstantsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::LatticeConstants));
    Float32Array::store_type* latticeConstants = latticeConstantsArray.getDataStore();

    crystalStructures[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
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
      crystalStructures[phaseId] = phase->determineLaueGroup();
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

  virtual Result<> copyRawEbsdData(int index) = 0;

protected:
  std::shared_ptr<T> m_Reader;
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  const ReadH5DataInputValues* m_InputValues;
};

} // namespace nx::core
