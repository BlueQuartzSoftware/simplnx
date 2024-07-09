#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadVtkStructuredPointsInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  bool ReadPointData;
  bool ReadCellData;
  DataPath PointGeomPath;
  DataPath ImageGeomPath;
  std::string PointAttributeMatrixName;
  std::string CellAttributeMatrixName;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ReadVtkStructuredPoints
{
public:
  ReadVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadVtkStructuredPointsInputValues* inputValues);
  ~ReadVtkStructuredPoints() noexcept;

  ReadVtkStructuredPoints(const ReadVtkStructuredPoints&) = delete;
  ReadVtkStructuredPoints(ReadVtkStructuredPoints&&) noexcept = delete;
  ReadVtkStructuredPoints& operator=(const ReadVtkStructuredPoints&) = delete;
  ReadVtkStructuredPoints& operator=(ReadVtkStructuredPoints&&) noexcept = delete;

  enum class ErrorCodes : int32
  {
    ConvertVtkDataTypeErr = -240,
    VtkReadBinaryDataErr = -241,
    ReadLineErr = -242,
    ReadStringEofErr = -243,
    ReadStringReadErr = -244,
    ReadStringLogicalIOErr = -245,
    ReadStringUnknownErr = -246,
    FileOpenErr = -247,
    FileTypeErr = -248,
    DatasetWordCountErr = -249,
    DatasetKeywordErr = -250,
    DatasetStructuredPtsErr = -251,
    DimsWordCountErr = -252,
    DimsKeywordErr = -253,
    SpacingWordCountErr = -254,
    SpacingKeywordErr = -255,
    OriginWordCountErr = -256,
    OriginKeywordErr = -257,
    DatasetTypeWordCountErr = -258,
    DatasetTypeKeywordErr = -259,
    MismatchedCellsAndTuplesErr = -260,
    MismatchedPointsAndTuplesErr = -261,
    UnknownSectionKeywordErr = -262,
    ReadScalarHeaderLineErr = -263,
    ReadScalarHeaderWordCountErr = -264,
    ReadLookupTableLineErr = -265,
    ReadLookupTableWordCountErr = -266,
    ReadLookupTableKeywordErr = -267,
    NumberConvertErr = -10351 // From DataArrayUtilities.hpp
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void setPreflight(bool value);

  enum class CurrentSectionType : uint32
  {
    NotSet = 0,
    Point = 1,
    Cell = 2,
  };

  nx::core::Result<OutputActions> getOutputActions() const
  {
    return m_OutputActions;
  }

  /**
   * @brief readFile Handles the main reading of the .vtk file
   * @return Integer error value
   */
  Result<> readFile();

protected:
  /**
   * @brief readData Reads a section of data from the .vtk file
   * @param instream Incoming file stream
   */
  virtual void readData(std::istream& instream);

  /**
   * @brief parseCoordinateLine Parses a line representing coordinates
   * @param input Incoming line to parse
   * @param value Coordinate value
   * @return Integer error value
   */
  //  Result<> parseCoordinateLine(const char* input, size_t& value);

  /**
   * @brief parseByteSize Parses the byte size from a data set declaration line
   * @param text Line to parse
   * @return Byte size result
   */
  //  Result<usize> parseByteSize(const std::string& text);

  /**
   * @brief readDataTypeSection Determines the type of data to be read from the .vtk file
   * @param in Incoming file stream
   * @param numPts Number of points to read
   * @param nextKeyWord Keyword for data type
   * @return Result object
   */
  Result<int32> readDataTypeSection(std::istream& in, int numPts, const std::string& nextKeyWord);

  /**
   * @brief readScalarData Reads scalar data attribute types
   * @param in Incoming file stream
   * @return Result object
   */
  Result<> readScalarData(std::istream& in, int numPts);

  /**
   * @brief readVectorData Reads vector data attribute types
   * @param in Incoming file stream
   * @param numPts Number of points
   * @return Result object
   */
  Result<> readVectorData(std::istream& in, int numPts);

  /**
   * @brief DecodeString Decodes a binary string from the .vtk file
   * @param resname Resulting decoded string
   * @param name Binary string to decode
   * @return Result object
   */
  //  Result<int32> DecodeString(char* resname, const char* name);

  void setComment(const std::string& comment);

  void setFileIsBinary(bool value);

  void setDatasetType(const std::string& dataSetType);

private:
  DataStructure& m_DataStructure;
  const ReadVtkStructuredPointsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::string m_Comment = {""};
  std::string m_DatasetType = {""};

  bool m_FileIsBinary = {true};

  bool m_Preflight = false;

  CurrentSectionType m_CurrentSectionType = CurrentSectionType::NotSet;
  CreateImageGeometryAction::DimensionType m_CurrentGeomDims;

  nx::core::Result<OutputActions> m_OutputActions;
};

} // namespace nx::core
