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

/**
 * @struct ReadVtkStructuredPointsInputValues
 * @brief Stores the input file, section choices, and destination names.
 */
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
 * @class ReadVtkStructuredPoints
 * @brief Reads legacy VTK STRUCTURED_POINTS data through bounded buffers.
 *
 * Preflight parses and skips selected data blocks while it creates output actions.
 * Execution converts ASCII or big-endian binary values into destination DataStores.
 */
class SIMPLNXCORE_EXPORT ReadVtkStructuredPoints
{
public:
  /**
   * @brief Creates a legacy VTK structured-points reader.
   * @param dataStructure Receives image geometries and arrays during execution.
   * @param mesgHandler Is retained but not used.
   * @param shouldCancel Stops before later data chunks or sections when true.
   * @param inputValues Specifies file and output settings. The caller must keep
   * this object alive for the reader lifetime.
   */
  ReadVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadVtkStructuredPointsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning reader.
   */
  ~ReadVtkStructuredPoints() noexcept;

  ReadVtkStructuredPoints(const ReadVtkStructuredPoints&) = delete;
  ReadVtkStructuredPoints(ReadVtkStructuredPoints&&) noexcept = delete;
  ReadVtkStructuredPoints& operator=(const ReadVtkStructuredPoints&) = delete;
  ReadVtkStructuredPoints& operator=(ReadVtkStructuredPoints&&) noexcept = delete;

  /**
   * @enum ErrorCodes
   * @brief Defines parser, conversion, and stream error values.
   */
  enum class ErrorCodes : int32
  {
    ConvertVtkDataTypeErr = -240,        ///< Reports an unsupported scalar type token.
    VtkReadBinaryDataErr = -241,         ///< Reports a truncated or oversized binary block.
    ReadLineErr = -242,                  ///< Reports an unexpected end while reading a line.
    ReadStringEofErr = -243,             ///< Reports an unexpected end while reading a token.
    ReadStringReadErr = -244,            ///< Reports a physical token-read failure.
    ReadStringLogicalIOErr = -245,       ///< Reports a logical token-stream failure.
    ReadStringUnknownErr = -246,         ///< Reports an unclassified token-read failure.
    FileOpenErr = -247,                  ///< Reports failure to open the input file.
    FileTypeErr = -248,                  ///< Reports an unsupported ASCII or binary declaration.
    DatasetWordCountErr = -249,          ///< Reports an invalid DATASET declaration length.
    DatasetKeywordErr = -250,            ///< Reports a missing DATASET keyword.
    DatasetStructuredPtsErr = -251,      ///< Reports a non-STRUCTURED_POINTS dataset.
    DimsWordCountErr = -252,             ///< Reports an invalid DIMENSIONS declaration length.
    DimsKeywordErr = -253,               ///< Reports a missing DIMENSIONS keyword.
    SpacingWordCountErr = -254,          ///< Reports an invalid SPACING declaration length.
    SpacingKeywordErr = -255,            ///< Reports a missing SPACING keyword.
    OriginWordCountErr = -256,           ///< Reports an invalid ORIGIN declaration length.
    OriginKeywordErr = -257,             ///< Reports a missing ORIGIN keyword.
    DatasetTypeWordCountErr = -258,      ///< Reports an invalid data-section declaration length.
    DatasetTypeKeywordErr = -259,        ///< Reports a missing POINT_DATA or CELL_DATA keyword.
    MismatchedCellsAndTuplesErr = -260,  ///< Reports a cell tuple-count mismatch.
    MismatchedPointsAndTuplesErr = -261, ///< Reports a point tuple-count mismatch.
    UnknownSectionKeywordErr = -262,     ///< Reports an unsupported attribute-section keyword.
    ReadScalarHeaderLineErr = -263,      ///< Reports failure to read an array header.
    ReadScalarHeaderWordCountErr = -264, ///< Reports insufficient array-header tokens.
    ReadLookupTableLineErr = -265,       ///< Reports failure to read a lookup-table header.
    ReadLookupTableWordCountErr = -266,  ///< Reports an invalid lookup-table declaration length.
    ReadLookupTableKeywordErr = -267,    ///< Reports a missing LOOKUP_TABLE keyword.
    AsciiDataReadErr = -268,             ///< Reports truncated or failed ASCII data.
    AsciiTokenTooLongErr = -269,         ///< Reports a token longer than 1,024 characters.
    AsciiStreamPositionErr = -270,       ///< Reports failure to restore the next header position.
    NumberConvertErr = -10351            ///< Reports numeric text conversion failure.
  };

  /**
   * @brief Parses the configured file in preflight or execution mode.
   * @return First file, syntax, conversion, or bulk-I/O error, or success after cancellation.
   *
   * Cancellation or an error can retain output actions or array chunks from earlier sections.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Selects output-action creation instead of destination-array writes.
   * @param value Enables preflight parsing when true.
   * @pre Call before operator() or readFile().
   */
  void setPreflight(bool value);

  /**
   * @enum CurrentSectionType
   * @brief Identifies the destination for the current attribute section.
   */
  enum class CurrentSectionType : uint32
  {
    NotSet = 0, ///< Selects no destination section.
    Point = 1,  ///< Selects point-data output.
    Cell = 2,   ///< Selects cell-data output.
  };

  nx::core::Result<OutputActions> getOutputActions() const
  {
    return m_OutputActions;
  }

  /**
   * @brief Parses headers and selected point or cell attribute sections.
   * @return First file, syntax, conversion, or bulk-I/O error, or success after cancellation.
   */
  Result<> readFile();

protected:
  /**
   * @brief Retains an inactive legacy extension point.
   * @param instream Is not read.
   * @warning The current implementation performs no work.
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
   * @brief Reads scalar or vector arrays until the next data section.
   * @param in Provides the VTK stream at an attribute keyword.
   * @param numPts Specifies tuples in the current section.
   * @param nextKeyWord Identifies the next point or cell section.
   * @return Next section tuple count, zero at end, or a parser error.
   */
  Result<int32> readDataTypeSection(std::istream& in, int32 numPts, const std::string& nextKeyWord);

  /**
   * @brief Reads one SCALARS declaration and its values.
   * @param in Provides the stream after the SCALARS keyword.
   * @param numPts Specifies tuples in the current section.
   * @return Header, conversion, skip, or bulk-I/O error, or success.
   */
  Result<> readScalarData(std::istream& in, int32 numPts);

  /**
   * @brief Reads one VECTORS declaration and its values.
   * @param in Provides the stream after the VECTORS keyword.
   * @param numPts Specifies tuples in the current section.
   * @return Header, conversion, skip, or bulk-I/O error, or success.
   */
  Result<> readVectorData(std::istream& in, int32 numPts);

  /**
   * @brief Creates or fills one declared VTK array.
   * @param in Provides the stream at array values.
   * @param numPts Specifies tuples in the current section.
   * @param name Specifies the array name.
   * @param scalarType Specifies the VTK scalar type token.
   * @param numComp Specifies components per tuple.
   * @return Type, skip, conversion, or bulk-I/O error, or success after cancellation.
   * @pre Preflight validates scalarType before execution.
   */
  Result<> readDataArray(std::istream& in, int32 numPts, const std::string& name, const std::string& scalarType, usize numComp);

  /**
   * @brief DecodeString Decodes a binary string from the .vtk file
   * @param resname Resulting decoded string
   * @param name Binary string to decode
   * @return Result object
   */
  //  Result<int32> DecodeString(char* resname, const char* name);

  /**
   * @brief Stores the parsed VTK comment line.
   * @param comment Specifies the file comment.
   */
  void setComment(const std::string& comment);

  /**
   * @brief Stores whether the current file uses binary data blocks.
   * @param value Selects binary blocks when true.
   */
  void setFileIsBinary(bool value);

  /**
   * @brief Stores the parsed VTK dataset token.
   * @param dataSetType Specifies the dataset token.
   */
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
