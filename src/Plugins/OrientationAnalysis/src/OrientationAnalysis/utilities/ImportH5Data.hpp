#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/StringArray.hpp"
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
  ImportH5Data(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ImportH5DataInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(mesgHandler)
  , m_InputValues(inputValues)
  {
    m_Reader = T::New();
    m_Reader->setFileName(inputValues->SelectedScanNames.inputFilePath.string());
  }

  virtual ~ImportH5Data() noexcept = default;

  ImportH5Data(const ImportH5Data&) = delete;
  ImportH5Data(ImportH5Data&&) noexcept = delete;
  ImportH5Data& operator=(const ImportH5Data&) = delete;
  ImportH5Data& operator=(ImportH5Data&&) noexcept = delete;

  Result<> execute()
  {
    auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
    imageGeom.setUnits(IGeometry::LengthUnit::Micrometer);

    int index = 0;
    for(const auto& currentScanName : m_InputValues->SelectedScanNames.scanNames)
    {
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
      return MakeErrorResult(-8970, "EbsdReader could not read the .h5 file.");
    }

    const auto phases = m_Reader->getPhaseVector();
    if(phases.empty())
    {
      return MakeErrorResult(-8971, "EbsdReader could not read the phase vector from the .h5 file.");
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
  const ImportH5DataInputValues* m_InputValues;
};

} // namespace complex
