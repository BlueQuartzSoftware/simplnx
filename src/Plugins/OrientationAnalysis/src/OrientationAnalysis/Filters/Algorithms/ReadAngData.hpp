#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/IFilter.hpp"

#include "EbsdLib/IO/TSL/AngReader.h"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ReadAngDataInputValues
{
  std::filesystem::path InputFile;
  DataPath DataContainerName;
  std::string CellAttributeMatrixName;
  std::string CellEnsembleAttributeMatrixName;
};

struct ORIENTATIONANALYSIS_EXPORT Ang_Private_Data
{
  std::array<size_t, 3> dims = {0, 0, 0};
  std::array<float, 3> resolution = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> origin = {0.0F, 0.0F, 0.0F};
  std::vector<AngPhase::Pointer> phases;
  int32_t units = 0;
};

/**
 * @brief The ReadAngDataPrivate class is a private implementation of the ReadAngData class
 */
class ORIENTATIONANALYSIS_EXPORT ReadAngDataPrivate
{
public:
  ReadAngDataPrivate(){};
  ~ReadAngDataPrivate() = default;

  ReadAngDataPrivate(const ReadAngDataPrivate&) = delete;            // Copy Constructor Not Implemented
  ReadAngDataPrivate(ReadAngDataPrivate&&) = delete;                 // Move Constructor Not Implemented
  ReadAngDataPrivate& operator=(const ReadAngDataPrivate&) = delete; // Copy Assignment Not Implemented
  ReadAngDataPrivate& operator=(ReadAngDataPrivate&&) = delete;      // Move Assignment Not Implemented

  Ang_Private_Data m_Data;

  std::string m_InputFile_Cache;
  fs::file_time_type m_TimeStamp_Cache;

#if 0
  void ReadDataFile(AngReader* reader, const std::string& m_InputFile, std::vector<size_t>& tDims, ANG_READ_FLAG flag)
  {
    QFileInfo fi(m_InputFile);
    QDateTime timeStamp(fi.lastModified());
    if(flag == ANG_FULL_FILE)
    {
      setInputFile_Cache(""); // We need something to trigger the file read below
    }

    // Drop into this if statement if we need to read from a file
    if(m_InputFile != getInputFile_Cache() || !getTimeStamp_Cache().isValid() || getTimeStamp_Cache() < timeStamp)
    {
      float zStep = 1.0, xOrigin = 0.0f, yOrigin = 0.0f, zOrigin = 0.0f;
      size_t zDim = 1;

      reader->setFileName(m_InputFile.toStdString());

      if(flag == ANG_HEADER_ONLY)
      {
        int32_t err = reader->readHeaderOnly();
        if(err < 0)
        {
          setErrorCondition(err, S2Q(reader->getErrorMessage()));
          setErrorCondition(getErrorCode(), "AngReader could not read the .ang file header.");
          m_FileWasRead = false;
          return;
        }

        m_FileWasRead = true;

        if(reader->getGrid() == EbsdLib::Ang::HexGrid)
        {
          setErrorCondition(-1000, "DREAM.3D does not directly read HEX grid .ang files. Please use the 'Convert Hexagonal "
                                   "Grid Data to Square Grid Data (TSL - .ang)' filter first to batch convert the Hex grid files.");
          m_FileWasRead = false;
          return;
        }
      }
      else
      {
        int32_t err = reader->readFile();
        if(err < 0)
        {
          setErrorCondition(err, S2Q(reader->getErrorMessage()));
          setErrorCondition(getErrorCode(), "AngReader could not read the .ang file.");
          return;
        }
      }
      tDims[0] = reader->getXDimension();
      tDims[1] = reader->getYDimension();
      tDims[2] = zDim; // We are reading a single slice

      Ang_Private_Data data;
      data.dims = {tDims[0], tDims[1], tDims[2]};
      data.resolution[0] = (reader->getXStep());
      data.resolution[1] = (reader->getYStep());
      data.resolution[2] = (zStep);
      data.origin[0] = (xOrigin);
      data.origin[1] = (yOrigin);
      data.origin[2] = (zOrigin);
      data.phases = reader->getPhaseVector();

      QString header = S2Q(reader->getOriginalHeader());
      if(header.contains("# TEM data") || header.contains("# File Created from ACOM RES results"))
      {
        data.units = 0;
      }
      else
      {
        data.units = 1;
      }

      setData(data);

      setInputFile_Cache(m_InputFile);

      QFileInfo newFi(m_InputFile);
      QDateTime timeStamp(newFi.lastModified());
      setTimeStamp_Cache(timeStamp);
    }
    else
    {
      m_FileWasRead = false;
    }

    // Read from cache

    Ang_Private_Data data = getData();
    tDims[0] = data.dims[0];
    tDims[1] = data.dims[1];
    tDims[2] = data.dims[2];
    ImageGeom::Pointer image = m->getGeometryAs<ImageGeom>();

    image->setDimensions(tDims[0], tDims[1], tDims[2]);
    image->setSpacing(data.resolution);
    image->setOrigin(data.origin);
    if(data.units == 0)
    {
      image->setUnits(IGeometry::LengthUnit::Nanometer);
    }
    else
    {
      image->setUnits(IGeometry::LengthUnit::Micrometer);
    }

    if(flag == ANG_FULL_FILE)
    {
      loadMaterialInfo(reader);
    }
  }

#endif
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT ReadAngData
{
public:
  ReadAngData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadAngDataInputValues* inputValues);
  ~ReadAngData() noexcept;

  ReadAngData(const ReadAngData&) = delete;            // Copy Constructor Not Implemented
  ReadAngData(ReadAngData&&) = delete;                 // Move Constructor Not Implemented
  ReadAngData& operator=(const ReadAngData&) = delete; // Copy Assignment Not Implemented
  ReadAngData& operator=(ReadAngData&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

protected:
private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadAngDataInputValues* m_InputValues = nullptr;

  /**
   * @brief
   * @param reader
   * @return Error code.
   */
  std::pair<int32, std::string> loadMaterialInfo(AngReader* reader) const;

  /**
   * @brief
   * @param reader
   */
  void copyRawEbsdData(AngReader* reader) const;
};

} // namespace complex
