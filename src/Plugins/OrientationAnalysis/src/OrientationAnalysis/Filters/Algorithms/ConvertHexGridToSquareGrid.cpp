#include "ConvertHexGridToSquareGrid.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Utilities/FilePathGenerator.hpp"

#include "EbsdLib/IO/HKL/CtfConstants.h"
#include "EbsdLib/IO/TSL/AngConstants.h"
#include "EbsdLib/IO/TSL/AngReader.h"

#include <fstream>
#include <sstream>

using namespace complex;

namespace
{

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
std::string modifyAngHeaderLine(std::string& buf)
{
  std::string line = "";
  if(buf.at(0) != '#')
  {
    line = buf;
    m_HeaderIsComplete = true;
    return line;
  }
  if(buf.at(0) == '#' && buf.size() == 1)
  {
    line = buf;
    return line;
  }
  // Start at the first character and walk until you find another non-space character
  usize i = 1;
  while(buf.at(i) == ' ')
  {
    ++i;
  }
  usize wordStart = i;
  // size_t wordEnd = i + 1;
  while(true)
  {
    if(buf.at(i) == 45 || buf.at(i) == 95)
    {
      ++i;
    } // "-" or "_" character
    else if(buf.at(i) >= 65 && buf.at(i) <= 90)
    {
      ++i;
    } // Upper case alpha character
    else if(buf.at(i) >= 97 && buf.at(i) <= 122)
    {
      ++i;
    } // Lower case alpha character
    else
    {
      break;
    }
  }
  // wordEnd = i;

  std::string word(buf.mid(wordStart));

  if(word.size() == 0)
  {
    line = buf;
    return line;
  }
  if(buf.contains(EbsdLib::Ang::HexGrid))
  {
    line = buf.replace(EbsdLib::Ang::HexGrid, EbsdLib::Ang::SquareGrid);
    // line = "# " + word + ": SqrGrid";
  }
  else if(buf.contains(EbsdLib::Ang::XStep))
  {
    line = "# " + EbsdLib::Ang::XStep + ": " + std::string::number((double)m_XResolution);
  }
  else if(buf.contains(EbsdLib::Ang::YStep))
  {
    line = "# " + EbsdLib::Ang::YStep + ": " + std::string::number((double)m_YResolution);
  }
  else if(buf.contains(EbsdLib::Ang::NColsOdd))
  {
    line = "# " + EbsdLib::Ang::NColsOdd + ": " + std::string::number(m_NumCols);
  }
  else if(buf.contains(EbsdLib::Ang::NColsEven))
  {
    line = "# " + EbsdLib::Ang::NColsEven + ": " + std::string::number(m_NumCols);
  }
  else if(buf.contains(EbsdLib::Ang::NRows))
  {
    line = "# " + EbsdLib::Ang::NRows + ": " + std::string::number(m_NumRows);
  }
  else
  {
    line = buf;
  }
  return line;
}

constexpr ChoicesParameter::ValueType k_ToScalarVector = 0;
constexpr ChoicesParameter::ValueType k_ToVectorScalar = 1;

class ConvertHexGridToSquareGridImpl
{

public:
  ConvertHexGridToSquareGridImpl(const Float32Array& inputQuat, Float32Array& outputQuat, ChoicesParameter::ValueType conversionType, const std::atomic_bool& shouldCancel)
  : m_Input(inputQuat)
  , m_Output(outputQuat)
  , m_ConversionType(conversionType)
  , m_ShouldCancel(shouldCancel)
  {
  }
  virtual ~ConvertHexGridToSquareGridImpl() = default;

  ConvertHexGridToSquareGridImpl(const ConvertHexGridToSquareGridImpl&) = default;           // Copy Constructor Default Implemented
  ConvertHexGridToSquareGridImpl(ConvertHexGridToSquareGridImpl&&) = delete;                 // Move Constructor Not Implemented
  ConvertHexGridToSquareGridImpl& operator=(const ConvertHexGridToSquareGridImpl&) = delete; // Copy Assignment Not Implemented
  ConvertHexGridToSquareGridImpl& operator=(ConvertHexGridToSquareGridImpl&&) = delete;      // Move Assignment Not Implemented

  void convert(usize start, usize end) const
  {
    // Let's assume k_ToScalarVector which means the incoming quaternions are Vector-Scalar
    // <x,y,z> w  ---> w <x,y,z>
    std::array<usize, 4> mapping = {{1, 2, 3, 0}};

    if(m_ConversionType == ::k_ToVectorScalar) // Ensure the Quaternion is the proper order
    {
      // w <x,y,z>  ---> <x,y,z> w
      mapping = {{3, 0, 1, 2}};
    }

    std::array<float, 4> temp = {0.0f, 0.0f, 0.0f, 0.0f};
    for(usize i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      temp[mapping[0]] = m_Input[i * 4];
      temp[mapping[1]] = m_Input[i * 4 + 1];
      temp[mapping[2]] = m_Input[i * 4 + 2];
      temp[mapping[3]] = m_Input[i * 4 + 3];

      m_Output[i * 4] = temp[0];
      m_Output[i * 4 + 1] = temp[1];
      m_Output[i * 4 + 2] = temp[2];
      m_Output[i * 4 + 3] = temp[3];
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const Float32Array& m_Input;
  Float32Array& m_Output;
  ChoicesParameter::ValueType m_ConversionType = 0;
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace

// -----------------------------------------------------------------------------
ConvertHexGridToSquareGrid::ConvertHexGridToSquareGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertHexGridToSquareGridInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConvertHexGridToSquareGrid::~ConvertHexGridToSquareGrid() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConvertHexGridToSquareGrid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGrid::operator()()
{
  bool hasMissingFiles = false;
  bool stackLowToHigh = true;
  int increment = 1;

  // Now generate all the file names the user is asking for and populate the table
  std::vector<std::string> fileList =
      FilePathGenerator::GenerateFileList(m_ZStartIndex, m_ZEndIndex, increment, hasMissingFiles, stackLowToHigh, m_InputPath, m_FilePrefix, m_FileSuffix, m_FileExtension, m_PaddingDigits);

  // Loop on Each EBSD File
  auto total = static_cast<float32>(m_ZEndIndex - m_ZStartIndex);
  int32 progress = 0;
  int64 z = m_ZStartIndex;
  /* There is a frailness about the z index and the file list. The programmer
   * using this code MUST ensure that the list of files that is sent into this
   * class is in the appropriate order to match up with the z index (slice index)
   * otherwise the import will have subtle errors. The programmer is urged NOT to
   * simply gather a list from the file system as those lists are sorted in such
   * a way that if the number of digits appearing in the filename are NOT the same
   * then the list will be wrong, ie, this example:
   *
   * slice_1.ang
   * slice_2.ang
   * ....
   * slice_10.ang
   *
   * Most, if not ALL C++ libraries when asked for that list will return the list
   * sorted like the following:
   *
   * slice_1.ang
   * slice_10.ang
   * slice_2.ang
   *
   * which is going to cause problems because the data is going to be placed
   * into the HDF5 file at the wrong index. YOU HAVE BEEN WARNED.
   */
  // int totalSlicesImported = 0;
  for(std::vector<std::string>::iterator filepath = fileList.begin(); filepath != fileList.end(); ++filepath)
  {
    std::string ebsdFName = *filepath;

    // Write the Manufacturer of the OIM file here
    // This list will grow to be the number of EBSD file formats we support
    QFileInfo fi(ebsdFName);
    std::string ext = fi.suffix().toStdString();
    std::string base = fi.baseName();
    QDir path(getOutputPath());
    if(ext == EbsdLib::Ang::FileExt)
    {
      AngReader reader;
      reader.setFileName(ebsdFName);
      reader.setReadHexGrid(true);
      int32 err = reader.readFile();
      if(err < 0 && err != -600)
      {
        return MakeErrorResult(reader.getErrorCode(), reader.getErrorMessage());
      }
      if(reader.getGrid().find(EbsdLib::Ang::SquareGrid) == 0)
      {
        return MakeErrorResult(-55000, fmt::format("Ang file is already a square grid: {}", ebsdFName));
      }

      if(err == -600)
      {
        setWarningCondition(reader.getErrorCode(), reader.getErrorMessage());
      }

      std::string origHeader = reader.getOriginalHeader();
      if(origHeader.empty())
      {
        return MakeErrorResult(-55001, fmt::format("Header could not be retrieved: {}",  ebsdFName));
      }

      std::stringstream in(&origHeader);
      // std::string newEbsdFName = path.absolutePath() + "/Sqr_" + base + "." + ext;
      std::string newEbsdFName = path.absolutePath() + "/" + getOutputPrefix() + base + "." + ext;

      if(newEbsdFName.compare(ebsdFName) == 0)
      {
        return MakeErrorResult(-201, "New ang file is the same as the old ang file. Overwriting is NOT allowed");
      }

      QFile outFile(newEbsdFName);
      QFileInfo fi(newEbsdFName);
      // Ensure the output path exists by creating it if necessary
      QDir parent(fi.absolutePath());
      if(!parent.exists())
      {
        parent.mkpath(fi.absolutePath());
      }

      if(!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
      {
        return MakeErrorResult(-200, fmt::format("Ang square output file could not be opened for writing: {}", newEbsdFName));
      }

      std::ofstream dStream(&outFile);

      m_HeaderIsComplete = false;

      float32 HexXStep = reader.getXStep();
      float32 HexYStep = reader.getYStep();
      int32 HexNumColsOdd = reader.getNumOddCols();
      int32 HexNumColsEven = reader.getNumEvenCols();
      int32 HexNumRows = reader.getNumRows();
      m_NumCols = (HexNumColsOdd * HexXStep) / m_XResolution;
      m_NumRows = (HexNumRows * HexYStep) / m_YResolution;
      float32 xSqr = 0.0f, ySqr = 0.0f, xHex1 = 0.0f, yHex1 = 0.0f, xHex2 = 0.0f, yHex2 = 0.0f;
      int32 point = 0, point1 = 0, point2 = 0;
      int32 row1 = 0, row2 = 0, col1 = 0, col2 = 0;
      float32 dist1 = 0.0f, dist2 = 0.0f;
      float32* phi1 = reader.getPhi1Pointer();
      float32* PHI = reader.getPhiPointer();
      float32* phi2 = reader.getPhi2Pointer();
      float32* ci = reader.getConfidenceIndexPointer();
      float32* iq = reader.getImageQualityPointer();
      float32* semsig = reader.getSEMSignalPointer();
      float32* fit = reader.getFitPointer();
      int32* phase = reader.getPhaseDataPointer();
      while(!in.atEnd())
      {
        std::string buf = in.readLine();
        std::string line = modifyAngHeaderLine(buf);
        if(!m_HeaderIsComplete)
        {
          dStream << line << "\n";
        }
      }
      for(int32 j = 0; j < m_NumRows; j++)
      {
        for(int32 i = 0; i < m_NumCols; i++)
        {
          xSqr = static_cast<float32>(i) * m_XResolution;
          ySqr = static_cast<float32>(j) * m_YResolution;
          row1 = ySqr / (HexYStep);
          yHex1 = row1 * HexYStep;
          row2 = row1 + 1;
          yHex2 = row2 * HexYStep;
          if(row1 % 2 == 0)
          {
            col1 = xSqr / (HexXStep);
            xHex1 = col1 * HexXStep;
            point1 = ((row1 / 2) * HexNumColsEven) + ((row1 / 2) * HexNumColsOdd) + col1;
            col2 = (xSqr - (HexXStep / 2.0)) / (HexXStep);
            xHex2 = col2 * HexXStep + (HexXStep / 2.0);
            point2 = ((row1 / 2) * HexNumColsEven) + (((row1 / 2) + 1) * HexNumColsOdd) + col2;
          }
          else
          {
            col1 = (xSqr - (HexXStep / 2.0)) / (HexXStep);
            xHex1 = col1 * HexXStep + (HexXStep / 2.0);
            point1 = ((row1 / 2) * HexNumColsEven) + (((row1 / 2) + 1) * HexNumColsOdd) + col1;
            col2 = xSqr / (HexXStep);
            xHex2 = col2 * HexXStep;
            point2 = (((row1 / 2) + 1) * HexNumColsEven) + (((row1 / 2) + 1) * HexNumColsOdd) + col2;
          }
          dist1 = ((xSqr - xHex1) * (xSqr - xHex1)) + ((ySqr - yHex1) * (ySqr - yHex1));
          dist2 = ((xSqr - xHex2) * (xSqr - xHex2)) + ((ySqr - yHex2) * (ySqr - yHex2));
          if(dist1 <= dist2 || row1 == (HexNumRows - 1))
          {
            point = point1;
          }
          else
          {
            point = point2;
          }
          dStream << "  " << phi1[point] << "	" << PHI[point] << "	" << phi2[point] << "	" << xSqr << "	" << ySqr << "	" << iq[point] << "	" << ci[point] << "	" << phase[point] << "	"
                  << semsig[point] << "	" << fit[point] << "	"
                  << "\n";
        }
      }
    }
    else if(ext.compare(EbsdLib::Ctf::FileExt) == 0)
    {
      return MakeErrorResult(-44601, "Ctf files are not on a hexagonal grid and do not need to be converted");
    }
    else
    {
      return MakeErrorResult(-44600, "The file extension was not detected correctly");
    }

    {
      progress = static_cast<int32_t>(z - m_ZStartIndex);
      progress = (int32_t)(100.0f * (float)(progress) / total);
      std::string msg = "Converted File: " + ebsdFName;
      notifyStatusMessage(msg.toLatin1().data());
    }

    if(getCancel())
    {
      break;
    }

  }

  return {};
}
