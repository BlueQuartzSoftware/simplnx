#include "ReadVtkStructuredPoints.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/Utilities/VtkUtilities.hpp"


using namespace nx::core;

namespace {


// -----------------------------------------------------------------------------
int32_t readHeader()
{
  return 0;
}


}

// -----------------------------------------------------------------------------
ReadVtkStructuredPoints::ReadVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ReadVtkStructuredPointsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadVtkStructuredPoints::~ReadVtkStructuredPoints() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadVtkStructuredPoints::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReadVtkStructuredPoints::operator()()
{


  return {};
}

// -----------------------------------------------------------------------------
int32_t ReadVtkStructuredPoints::readFile(ReadVtkStructuredPointsInputValues* inputValues)
{
  int32_t err = 0;

  DataContainer::Pointer volDc = getDataContainerArray()->getDataContainer(getVolumeDataContainerName());
  AttributeMatrix::Pointer volAm = volDc->getAttributeMatrix(getCellAttributeMatrixName());

  DataContainer::Pointer vertDc = getDataContainerArray()->getDataContainer(getVertexDataContainerName());
  AttributeMatrix::Pointer vertAm = vertDc->getAttributeMatrix(getVertexAttributeMatrixName());


  std::ifstream in(inputValues->InputFile.c_str(), std::ios_base::in | std::ios_base::binary);

  if(!in.is_open())
  {
    std::string msg = QObject::tr("Error opening output file '%1'").arg(getInputFile());
    setErrorCondition(-61003, msg);
    return -100;
  }

  std::vector<char >  buffer (kBufferSize, '\0');

  std::fill(buffer.begin(), buffer.end(), '\0');
  err = readLine(in, buffer, kBufferSize); // Read Line 1 - VTK Version Info
  std::fill(buffer.begin(), buffer.end(), '\0');
  err = readLine(in, buffer, kBufferSize); // Read Line 2 - User Comment
  std::fill(buffer.begin(), buffer.end(), '\0');
  //setComment(std::string(buffer.data()));
  err = readLine(in, buffer, kBufferSize); // Read Line 3 - BINARY or ASCII

  std::string fileType(buffer.data());
  if(nx::core::StringUtilities::starts_with(fileType, "BINARY"))
  {
    setFileIsBinary(true);
  }
  else if(fileType.startsWith("ASCII"))
  {
    setFileIsBinary(false);
  }
  else
  {
    std::string ss = QObject::tr("The file type of the VTK legacy file could not be determined. It should be 'ASCII' or 'BINARY' and should appear on line 3 of the file");
    setErrorCondition(-61004, ss);
    return getErrorCode();
  }

  // Read Line 4 - Type of Dataset
  std::fill(buffer.begin(), buffer.end(), '\0');
  err = readLine(in, buffer, kBufferSize);

  std::vector<std::string> words = nx::core::StringUtilities::split({buffer.data()}, ' ');

  if(words.size() != 2)
  {
    std::string ss = QObject::tr("Error reading the type of data set. Was expecting 2 words but got %1").arg(std::string(buf));
    setErrorCondition(-61005, ss);
    return getErrorCode();
  }
  std::string dataset(words.at(1));
  dataset = nx::core::StringUtilities::trimmed(dataset);
  setDatasetType(dataset); // Should be STRUCTURED_POINTS

  bool ok = false;
  err = readLine(in, buffer, kBufferSize); // Read Line 5 which is the Dimension values
  // But we need the 'extents' which is one less in all directions (unless dim=1)
  std::vector<size_t> dims(3, 0);
  std::fill(buffer.begin(), buffer.end(), '\0');
  std::string line(buffer.data());
  std::vector<std::string> tokens = nx::core::StringUtilities::split(line, ' ');
  dims[0] = tokens[1].toInt(&ok, 10);
  dims[1] = tokens[2].toInt(&ok, 10);
  dims[2] = tokens[3].toInt(&ok, 10);
  std::vector<size_t> tDims(3, 0);
  tDims[0] = dims[0];
  tDims[1] = dims[1];
  tDims[2] = dims[2];
  vertAm->setTupleDimensions(tDims);
  vertDc->getGeometryAs<ImageGeom>()->setDimensions(dims.data());

  tDims[0] = dims[0] - 1;
  tDims[1] = dims[1] - 1;
  tDims[2] = dims[2] - 1;
  volAm->setTupleDimensions(tDims);
  volDc->getGeometryAs<ImageGeom>()->setDimensions(tDims.data());

  std::fill(buffer.begin(), buffer.end(), '\0');
  err = readLine(in, buffer, kBufferSize); // Read Line 7 which is the Scaling values
  line = std::string(buffer.data());
  tokens = tokens = nx::core::StringUtilities::split(line, ' ');
  float resolution[3];
  resolution[0] = tokens[1].toFloat(&ok);
  resolution[1] = tokens[2].toFloat(&ok);
  resolution[2] = tokens[3].toFloat(&ok);

  volDc->getGeometryAs<ImageGeom>()->setSpacing(resolution);
  vertDc->getGeometryAs<ImageGeom>()->setSpacing(resolution);

  std::fill(buffer.begin(), buffer.end(), '\0');
  err = readLine(in, buffer, kBufferSize); // Read Line 6 which is the Origin values
  line = std::string(buffer.data());
  tokens = tokens = nx::core::StringUtilities::split(line, ' ');
  float origin[3];
  origin[0] = tokens[1].toFloat(&ok);
  origin[1] = tokens[2].toFloat(&ok);
  origin[2] = tokens[3].toFloat(&ok);

  volDc->getGeometryAs<ImageGeom>()->setOrigin(origin);
  vertDc->getGeometryAs<ImageGeom>()->setOrigin(origin);

  // Read the first key word which should be POINT_DATA or CELL_DATA
  std::fill(buffer.begin(), buffer.end(), '\0');
  err = readLine(in, buffer, kBufferSize); // Read Line 6 which is the first type of data we are going to read
  line = std::string(buffer.data());
  tokens = tokens = nx::core::StringUtilities::split(line, ' ');
  std::string word = std::string(tokens[0]);
  int32_t npts = 0, ncells = 0;
  int32_t numPts = 0;

  if(word.startsWith("CELL_DATA"))
  {
    DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getVolumeDataContainerName());
    m_CurrentAttrMat = m->getAttributeMatrix(getCellAttributeMatrixName());
    ncells = tokens[1].toInt(&ok);
    if(m_CurrentAttrMat->getNumberOfTuples() != ncells)
    {
      setErrorCondition(-61006, std::string("Number of cells does not match number of tuples in the Attribute Matrix"));
      return getErrorCode();
    }
    readDataTypeSection(in, ncells, "point_data");
  }
  else if(word.startsWith("POINT_DATA"))
  {
    DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getVertexDataContainerName());
    m_CurrentAttrMat = m->getAttributeMatrix(getVertexAttributeMatrixName());
    npts = tokens[1].toInt(&ok);
    if(m_CurrentAttrMat->getNumberOfTuples() != npts)
    {
      setErrorCondition(-61007, std::string("Number of points does not match number of tuples in the Attribute Matrix"));
      return getErrorCode();
    }
    readDataTypeSection(in, numPts, "cell_data");
  }

  // Close the file since we are done with it.
  in.close();

  return err;
}

