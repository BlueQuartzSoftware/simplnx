#include "ConvertHexGridToSquareGrid.hpp"

#include "complex/Common/AtomicFile.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Utilities/FilePathGenerator.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include "EbsdLib/IO/TSL/AngConstants.h"
#include "EbsdLib/IO/TSL/AngReader.h"

#include <fstream>
#include <sstream>

using namespace complex;

namespace
{
constexpr ChoicesParameter::ValueType k_ToScalarVector = 0;
constexpr ChoicesParameter::ValueType k_ToVectorScalar = 1;

class Converter
{
public:
  Converter(const std::atomic_bool& shouldCancel, const fs::path& outputDirPath, const std::string& outputFilePrefix, const std::vector<float32>& spacingXY)
  : m_ShouldCancel(shouldCancel)
  , m_OutputPath(outputDirPath)
  , m_FilePrefix(outputFilePrefix)
  , m_SqrXStep(spacingXY.at(0))
  , m_SqrYStep(spacingXY.at(1))
  {
  }
  ~Converter() noexcept
  {
    for(const auto& atomicFile : m_AtomicFiles)
    {
      atomicFile.commit();
    }
  }

  Converter(const Converter&) = delete;            // Copy Constructor Default Implemented
  Converter(Converter&&) = delete;                 // Move Constructor Not Implemented
  Converter& operator=(const Converter&) = delete; // Copy Assignment Not Implemented
  Converter& operator=(Converter&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()(const fs::path& inputPath)
  {
    // Write the Manufacturer of the OIM file here
    // This list will grow to be the number of EBSD file formats we support
    if(inputPath.extension() == ("." + EbsdLib::Ang::FileExt))
    {
      AngReader reader;
      reader.setFileName(inputPath.string());
      reader.setReadHexGrid(true);
      int32 err = reader.readFile();
      if(err < 0 && err != -600)
      {
        return MakeErrorResult(reader.getErrorCode(), reader.getErrorMessage());
      }
      if(err == -600)
      {
        return MakeWarningVoidResult(reader.getErrorCode(), reader.getErrorMessage());
      }
      if(reader.getGrid().find(EbsdLib::Ang::SquareGrid) == 0)
      {
        return MakeErrorResult(-55000, fmt::format("Ang file is already a square grid: {}", inputPath.string()));
      }

      std::string origHeader = reader.getOriginalHeader();
      if(origHeader.empty())
      {
        return MakeErrorResult(-55001, fmt::format("Header for input hex grid file was empty: {}", inputPath.string()));
      }

      std::istringstream headerStream(origHeader, std::ios_base::in | std::ios_base::binary);
      m_AtomicFiles.emplace_back((fs::absolute(m_OutputPath) / (m_FilePrefix + inputPath.filename().string())), false);
      fs::path outPath = m_AtomicFiles[m_Index].tempFilePath();

      if(outPath == inputPath)
      {
        return MakeErrorResult(-201, "New ang file is the same as the old ang file. Overwriting is NOT allowed");
      }

      std::ofstream outFile(outPath, std::ios_base::out | std::ios_base::binary);
      // Ensure the output path exists by creating it if necessary
      if(!fs::exists(outPath.parent_path()))
      {
        return MakeErrorResult(-77750, fmt::format("The parent path was not created and does not exist: {}", outPath.parent_path().string()));
      }

      if(!outFile.is_open())
      {
        return MakeErrorResult(-200, fmt::format("Ang square output file could not be opened for writing: {}", outPath.string()));
      }

      m_HeaderIsComplete = false;

      float32 hexXStep = reader.getXStep();
      float32 hexYStep = reader.getYStep();
      int32 hexNumColsOdd = reader.getNumOddCols();
      int32 hexNumColsEven = reader.getNumEvenCols();
      int32 hexNumRows = reader.getNumRows();
      m_SqrNumCols = static_cast<int32>((static_cast<float32>(hexNumColsOdd) * hexXStep) / m_SqrXStep);
      m_SqrNumRows = static_cast<int32>((static_cast<float32>(hexNumRows) * hexYStep) / m_SqrYStep);

      // Get the hex grid data from the input file
      float32* phi1 = reader.getPhi1Pointer();
      float32* PHI = reader.getPhiPointer();
      float32* phi2 = reader.getPhi2Pointer();
      float32* ci = reader.getConfidenceIndexPointer();
      float32* iq = reader.getImageQualityPointer();
      float32* semSig = reader.getSEMSignalPointer();
      float32* fit = reader.getFitPointer();
      int32* phase = reader.getPhaseDataPointer();

      // Loop on the header part of the file and modify the header while writing to the output file
      // the updated information
      while(!headerStream.eof())
      {
        std::string buf;
        std::getline(headerStream, buf);
        if(buf.empty())
        {
          continue;
        }
        std::string line = modifyAngHeaderLine(buf);
        if(!m_HeaderIsComplete)
        {
          outFile << line << "\n";
        }
      }

      // Reusable variables for the loop
      int32 hexGridIndex, hexGridIndex1, hexGridIndex2;
      float32 xHex1, xHex2;

      // Now loop on every cell in the replacement square grid
      for(int32 j = 0; j < m_SqrNumRows; j++)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        for(int32 i = 0; i < m_SqrNumCols; i++)
        {
          // Calculate the center point of the current Square grid cell
          float32 xSqr = static_cast<float32>(i) * m_SqrXStep;
          float32 ySqr = static_cast<float32>(j) * m_SqrYStep;

          // Calculate hex grid rows that will be used to for the conversion for this square grid point
          int32 hexRowIndex1 = static_cast<int32>(ySqr / hexYStep);
          float32 yHex1 = static_cast<float32>(hexRowIndex1) * hexYStep;

          int32 hexRowIndex2 = hexRowIndex1 + 1;
          float32 yHex2 = static_cast<float32>(hexRowIndex2) * hexYStep;

          // If we are on an even row
          if(hexRowIndex1 % 2 == 0)
          {
            int32 hexGridColIndex1 = static_cast<int32>(xSqr / (hexXStep));                                                  // Compute the hex column index that the square grid coord falls into
            xHex1 = static_cast<float32>(hexGridColIndex1) * hexXStep;                                                       // Compute the X Hex Grid coordinate
            hexGridIndex1 = ((hexRowIndex1 / 2) * hexNumColsEven) + ((hexRowIndex1 / 2) * hexNumColsOdd) + hexGridColIndex1; // Compute the index into hex grid data

            int32 hexGridColIndex2 = static_cast<int32>((xSqr - (hexXStep / 2.0)) / (hexXStep));
            xHex2 = static_cast<float32>(static_cast<float32>(hexGridColIndex2) * hexXStep + (hexXStep / 2.0));
            hexGridIndex2 = ((hexRowIndex1 / 2) * hexNumColsEven) + (((hexRowIndex1 / 2) + 1) * hexNumColsOdd) + hexGridColIndex2;
          }
          else // If we are on an odd row
          {
            int32 hexGridColIndex1 = static_cast<int32>((xSqr - (hexXStep / 2.0)) / (hexXStep));
            xHex1 = static_cast<float32>(static_cast<float32>(hexGridColIndex1) * hexXStep + (hexXStep / 2.0));
            hexGridIndex1 = ((hexRowIndex1 / 2) * hexNumColsEven) + (((hexRowIndex1 / 2) + 1) * hexNumColsOdd) + hexGridColIndex1;

            int32 hexGridColIndex2 = static_cast<int32>(xSqr / hexXStep);
            xHex2 = static_cast<float32>(hexGridColIndex2) * hexXStep;
            hexGridIndex2 = (((hexRowIndex1 / 2) + 1) * hexNumColsEven) + (((hexRowIndex1 / 2) + 1) * hexNumColsOdd) + hexGridColIndex2;
          }

          // Compute the distance from the square grid coordinate to the hex grid coordinate.
          float32 dist1 = ((xSqr - xHex1) * (xSqr - xHex1)) + ((ySqr - yHex1) * (ySqr - yHex1));
          float32 dist2 = ((xSqr - xHex2) * (xSqr - xHex2)) + ((ySqr - yHex2) * (ySqr - yHex2));
          // Select the closest hex grid point.
          if(dist1 <= dist2 || hexRowIndex1 == (hexNumRows - 1))
          {
            hexGridIndex = hexGridIndex1;
          }
          else
          {
            hexGridIndex = hexGridIndex2;
          }

          // Write to the output file
          outFile << "  " << phi1[hexGridIndex] << "	" << PHI[hexGridIndex] << "	" << phi2[hexGridIndex] << "	" << xSqr << "	" << ySqr << "	" << iq[hexGridIndex] << "	" << ci[hexGridIndex]
                  << "	" << phase[hexGridIndex] << "	" << semSig[hexGridIndex] << "	" << fit[hexGridIndex] << "  "
                  << "\n";
        }
      }

      m_Index++;
    }

    return {};
  }

private:
  const std::atomic_bool& m_ShouldCancel;
  const fs::path& m_OutputPath;
  const std::string& m_FilePrefix;
  std::vector<AtomicFile> m_AtomicFiles = {};
  usize m_Index = 0;

  const float32 m_SqrXStep;
  const float32 m_SqrYStep;
  int32 m_SqrNumCols = 0;
  int32 m_SqrNumRows = 0;
  bool m_HeaderIsComplete = false;

  // -----------------------------------------------------------------------------
  std::string modifyAngHeaderLine(std::string& buf)
  {
    std::string line;
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
    while(true)
    {
      if(buf.at(i) == 45 || buf.at(i) == 95 || (buf.at(i) >= 65 && buf.at(i) <= 90) || (buf.at(i) >= 97 && buf.at(i) <= 122))
      {
        ++i;
      } // "-" or "_" character
      else
      {
        break;
      }
    }

    std::string word(buf.substr(wordStart));

    if(word.empty())
    {
      line = buf;
      return line;
    }

    if(StringUtilities::contains(buf, EbsdLib::Ang::HexGrid))
    {
      line = StringUtilities::replace(buf, EbsdLib::Ang::HexGrid, EbsdLib::Ang::SquareGrid);
    }
    else if(StringUtilities::contains(buf, EbsdLib::Ang::XStep))
    {
      line = "# " + EbsdLib::Ang::XStep + ": " + StringUtilities::number(m_SqrXStep);
    }
    else if(StringUtilities::contains(buf, EbsdLib::Ang::YStep))
    {
      line = "# " + EbsdLib::Ang::YStep + ": " + StringUtilities::number(m_SqrYStep);
    }
    else if(StringUtilities::contains(buf, EbsdLib::Ang::NColsOdd))
    {
      line = "# " + EbsdLib::Ang::NColsOdd + ": " + StringUtilities::number(m_SqrNumCols);
    }
    else if(StringUtilities::contains(buf, EbsdLib::Ang::NColsEven))
    {
      line = "# " + EbsdLib::Ang::NColsEven + ": " + StringUtilities::number(m_SqrNumCols);
    }
    else if(StringUtilities::contains(buf, EbsdLib::Ang::NRows))
    {
      line = "# " + EbsdLib::Ang::NRows + ": " + StringUtilities::number(m_SqrNumRows);
    }
    else
    {
      line = buf;
    }

    return line;
  }
};
} // namespace

// -----------------------------------------------------------------------------
ConvertHexGridToSquareGrid::ConvertHexGridToSquareGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ConvertHexGridToSquareGridInputValues* inputValues)
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
  if(!m_InputValues->MultiFile)
  {
    return ::Converter(getCancel(), m_InputValues->OutputPath, m_InputValues->OutputFilePrefix, m_InputValues->XYSpacing)(m_InputValues->InputPath);
  }

  // Now generate all the file names the user is asking for and populate the table
  std::vector<std::string> fileList =
      FilePathGenerator::GenerateFileList(m_InputValues->InputFileListInfo.startIndex, m_InputValues->InputFileListInfo.endIndex, m_InputValues->InputFileListInfo.incrementIndex,
                                          m_InputValues->InputFileListInfo.ordering, m_InputValues->InputPath.string(), m_InputValues->InputFileListInfo.filePrefix,
                                          m_InputValues->InputFileListInfo.fileSuffix, m_InputValues->InputFileListInfo.fileExtension, m_InputValues->InputFileListInfo.paddingDigits);

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
  // Loop on Each EBSD File
  auto total = static_cast<float32>(m_InputValues->InputFileListInfo.endIndex - m_InputValues->InputFileListInfo.startIndex);
  int32 progress;
  int64 z = m_InputValues->InputFileListInfo.startIndex;

  usize index = 0;
  auto result = Result<>{};
  ::Converter converter(getCancel(), m_InputValues->OutputPath, m_InputValues->OutputFilePrefix, m_InputValues->XYSpacing);
  for(const auto& filepath : fileList)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Now Processing: {}", filepath));

    result = MergeResults(converter(filepath), result);
    index++;

    {
      z++;
      progress = static_cast<int32>(z - m_InputValues->InputFileListInfo.startIndex - 1);
      progress = static_cast<int32>(100.0f * static_cast<float32>(progress) / total);
      std::string msg = "Converted File: " + filepath;
      m_MessageHandler(IFilter::Message::Type::Progress, msg, progress);
    }

    if(getCancel())
    {
      result = MergeResults(MakeErrorResult(-00010, "Run Terminated Early"), result);
      return result;
    }
  }

  return result;
}
