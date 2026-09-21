#include "MetaImageUtilities.hpp"

#include "simplnx/Common/Bit.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <limits>

namespace fs = std::filesystem;

namespace nx::core::mhd
{
std::optional<std::pair<DataType, usize>> MetTypeToSimplnx(const std::string& metType)
{
  // metType is expected already trimmed + upper-cased (e.g. "MET_UCHAR").
  if(metType == "MET_CHAR")
  {
    return std::make_pair(DataType::int8, usize{1});
  }
  if(metType == "MET_UCHAR")
  {
    return std::make_pair(DataType::uint8, usize{1});
  }
  if(metType == "MET_SHORT")
  {
    return std::make_pair(DataType::int16, usize{2});
  }
  if(metType == "MET_USHORT")
  {
    return std::make_pair(DataType::uint16, usize{2});
  }
  if(metType == "MET_INT")
  {
    return std::make_pair(DataType::int32, usize{4});
  }
  if(metType == "MET_UINT")
  {
    return std::make_pair(DataType::uint32, usize{4});
  }
  // MetaIO defines MET_LONG / MET_ULONG as a fixed 4 bytes (MET_LONG_LONG is the
  // 8-byte variant), so map them to 32-bit to match the on-disk byte layout ITK
  // reads. See risk R1.
  if(metType == "MET_LONG")
  {
    return std::make_pair(DataType::int32, usize{4});
  }
  if(metType == "MET_ULONG")
  {
    return std::make_pair(DataType::uint32, usize{4});
  }
  if(metType == "MET_LONG_LONG")
  {
    return std::make_pair(DataType::int64, usize{8});
  }
  if(metType == "MET_ULONG_LONG")
  {
    return std::make_pair(DataType::uint64, usize{8});
  }
  if(metType == "MET_FLOAT")
  {
    return std::make_pair(DataType::float32, usize{4});
  }
  if(metType == "MET_DOUBLE")
  {
    return std::make_pair(DataType::float64, usize{8});
  }
  // MET_STRING / MET_OTHER / unknown are unsupported.
  return std::nullopt;
}

namespace
{
std::string StripCR(std::string s)
{
  if(!s.empty() && s.back() == '\r')
  {
    s.pop_back();
  }
  return s;
}

std::string Trim(const std::string& s)
{
  const auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };
  auto begin = std::find_if(s.begin(), s.end(), notSpace);
  auto end = std::find_if(s.rbegin(), s.rend(), notSpace).base();
  return (begin < end) ? std::string(begin, end) : std::string{};
}

std::string ToLower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string ToUpper(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return s;
}

std::vector<std::string> SplitWhitespace(const std::string& s)
{
  std::vector<std::string> out;
  std::string cur;
  for(char c : s)
  {
    if(std::isspace(static_cast<unsigned char>(c)) != 0)
    {
      if(!cur.empty())
      {
        out.push_back(cur);
        cur.clear();
      }
    }
    else
    {
      cur.push_back(c);
    }
  }
  if(!cur.empty())
  {
    out.push_back(cur);
  }
  return out;
}

std::optional<usize> ToUSize(const std::string& s)
{
  try
  {
    const std::string trimmed = Trim(s);
    size_t pos = 0;
    const unsigned long long v = std::stoull(trimmed, &pos);
    if(pos != trimmed.size())
    {
      return std::nullopt;
    }
    return static_cast<usize>(v);
  } catch(...)
  {
    return std::nullopt;
  }
}

// Parse a whitespace-separated list of floating-point values. Returns nullopt if
// any token fails to parse (so a malformed numeric field is a hard error).
std::optional<std::vector<float64>> ParseFloatList(const std::string& value)
{
  std::vector<float64> out;
  for(const auto& tok : SplitWhitespace(value))
  {
    try
    {
      size_t pos = 0;
      const float64 v = std::stod(tok, &pos);
      if(pos != tok.size())
      {
        return std::nullopt;
      }
      out.push_back(v);
    } catch(...)
    {
      return std::nullopt;
    }
  }
  return out;
}

// MetaImage booleans are decided by the FIRST non-space character only, exactly as
// MetaIO does (metaObject.cxx ~1491/1510/1523/1536): a leading 'T', 't', or '1' is
// true, anything else is false. This accepts "True"/"true"/"1" AND MetaIO's numeric
// form (e.g. "CompressedData = 1", "BinaryDataByteOrderMSB = T"); the old
// whole-string "== true" comparison silently mis-read those as false.
bool ParseBool(const std::string& value)
{
  const std::string trimmed = Trim(value);
  if(trimmed.empty())
  {
    return false;
  }
  const char c = trimmed.front();
  return c == 'T' || c == 't' || c == '1';
}

// Parses a signed integer field (HeaderSize is a MET_INT that MetaIO stores as an int,
// with -1 a legal sentinel). Returns nullopt if the whole trimmed token is not one integer.
std::optional<long> ToLong(const std::string& s)
{
  try
  {
    const std::string trimmed = Trim(s);
    size_t pos = 0;
    const long v = std::stol(trimmed, &pos);
    if(pos != trimmed.size())
    {
      return std::nullopt;
    }
    return v;
  } catch(...)
  {
    return std::nullopt;
  }
}
} // namespace

Result<MetaImageMetadata> ReadMetaImageHeader(const std::filesystem::path& filePath)
{
  const std::string pathStr = filePath.string();
  std::ifstream in(filePath, std::ios::binary);
  if(!in.is_open())
  {
    return MakeErrorResult<MetaImageMetadata>(-35800, fmt::format("Could not open MetaImage file for reading: '{}'", pathStr));
  }

  MetaImageMetadata md;
  md.filePath = pathStr;

  std::string metType;
  std::string elementDataFile;
  std::vector<usize> dimSize;
  std::vector<float64> elementSpacing;
  std::vector<float64> elementSizeFallback;
  // Origin aliases, resolved after the loop by MetaIO's fixed precedence
  // (ImagePosition > Origin > Offset > Position). MetaIO reads each into the same
  // m_Offset member in that code order (metaObject.cxx ~1566-1589, then metaImage.cxx
  // ~2262), so the last one read wins REGARDLESS of the order they appear in the file.
  std::optional<std::vector<float64>> offsetPosition;
  std::optional<std::vector<float64>> offsetOffset;
  std::optional<std::vector<float64>> offsetOrigin;
  std::optional<std::vector<float64>> offsetImagePosition;
  // Transform aliases, resolved by TransformMatrix > Rotation > Orientation
  // (metaObject.cxx ~1592-1621, same last-read-wins fixed order).
  std::optional<std::vector<float64>> xformOrientation;
  std::optional<std::vector<float64>> xformRotation;
  std::optional<std::vector<float64>> xformTransformMatrix;
  // Byte-order aliases, resolved by BinaryDataByteOrderMSB > ElementByteOrderMSB
  // (metaObject.cxx ~1520-1544: ElementByteOrderMSB is read first, then
  // BinaryDataByteOrderMSB overwrites it).
  std::optional<bool> msbElementByteOrder;
  std::optional<bool> msbBinaryDataByteOrder;
  usize nDims = 0;
  bool haveNDims = false;
  bool binaryData = true; // MetaImage default is binary
  bool compressed = false;
  bool haveDataFile = false;

  std::string line;
  while(std::getline(in, line))
  {
    line = StripCR(line);
    const std::string trimmedLine = Trim(line);
    if(trimmedLine.empty() || trimmedLine[0] == '#')
    {
      continue;
    }
    const auto eq = line.find('=');
    if(eq == std::string::npos)
    {
      continue; // not a Key = Value line
    }
    const std::string keyLower = ToLower(Trim(line.substr(0, eq)));
    const std::string value = Trim(line.substr(eq + 1));

    if(keyLower == "objecttype")
    {
      if(ToLower(value) != "image")
      {
        return MakeErrorResult<MetaImageMetadata>(-35802, fmt::format("MetaImage '{}' has ObjectType '{}'; only 'Image' is supported.", pathStr, value));
      }
    }
    else if(keyLower == "headersize")
    {
      // MetaIO honors HeaderSize in M_ReadElements (metaImage.cxx ~2352/2367): a value > 0
      // seeks to that absolute byte offset before reading the data, and -1 seeks to
      // (end - dataSize). This reader always reads from the post-header stream position
      // (attached) or offset 0 (detached), so a nonzero HeaderSize (including -1) would
      // silently read the wrong bytes. Reject it (fail loud) instead of producing wrong
      // pixels. HeaderSize = 0 is the default no-op and is accepted; a value that is not a
      // single integer is likewise refused rather than assumed harmless. Standard
      // ITK-written .mha omit HeaderSize, so mainstream files are unaffected.
      const auto v = ToLong(value);
      if(!v.has_value() || v.value() != 0)
      {
        return MakeErrorResult<MetaImageMetadata>(
            -35814, fmt::format("MetaImage '{}' specifies an unsupported HeaderSize of '{}'. This reader cannot honor a nonzero HeaderSize (MetaIO seeks to it before reading the data), so it "
                                "refuses rather than read the wrong voxels.",
                                pathStr, value));
      }
    }
    else if(keyLower == "ndims")
    {
      const auto v = ToUSize(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35806, fmt::format("MetaImage '{}' has non-numeric NDims '{}'.", pathStr, value));
      }
      nDims = v.value();
      haveNDims = true;
    }
    else if(keyLower == "dimsize")
    {
      for(const auto& tok : SplitWhitespace(value))
      {
        const auto v = ToUSize(tok);
        if(!v.has_value())
        {
          return MakeErrorResult<MetaImageMetadata>(-35806, fmt::format("MetaImage '{}' has non-numeric DimSize token '{}'.", pathStr, tok));
        }
        dimSize.push_back(v.value());
      }
    }
    else if(keyLower == "elementtype")
    {
      metType = ToUpper(Trim(value));
    }
    else if(keyLower == "elementnumberofchannels")
    {
      const auto v = ToUSize(value);
      if(!v.has_value() || v.value() == 0)
      {
        return MakeErrorResult<MetaImageMetadata>(-35806, fmt::format("MetaImage '{}' has invalid ElementNumberOfChannels '{}'.", pathStr, value));
      }
      md.componentCount = v.value();
    }
    else if(keyLower == "elementspacing")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed ElementSpacing '{}'.", pathStr, value));
      }
      elementSpacing = std::move(v.value());
    }
    else if(keyLower == "elementsize")
    {
      auto v = ParseFloatList(value);
      if(v.has_value())
      {
        elementSizeFallback = std::move(v.value());
      }
    }
    else if(keyLower == "position")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed Position '{}'.", pathStr, value));
      }
      offsetPosition = std::move(v.value());
    }
    else if(keyLower == "offset")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed Offset '{}'.", pathStr, value));
      }
      offsetOffset = std::move(v.value());
    }
    else if(keyLower == "origin")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed Origin '{}'.", pathStr, value));
      }
      offsetOrigin = std::move(v.value());
    }
    else if(keyLower == "imageposition")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed ImagePosition '{}'.", pathStr, value));
      }
      offsetImagePosition = std::move(v.value());
    }
    else if(keyLower == "binarydata")
    {
      binaryData = ParseBool(value);
    }
    else if(keyLower == "binarydatabyteordermsb")
    {
      msbBinaryDataByteOrder = ParseBool(value);
    }
    else if(keyLower == "elementbyteordermsb")
    {
      msbElementByteOrder = ParseBool(value);
    }
    else if(keyLower == "compresseddata")
    {
      compressed = ParseBool(value);
    }
    else if(keyLower == "transformmatrix")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed TransformMatrix '{}'.", pathStr, value));
      }
      xformTransformMatrix = std::move(v.value());
    }
    else if(keyLower == "rotation")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed Rotation '{}'.", pathStr, value));
      }
      xformRotation = std::move(v.value());
    }
    else if(keyLower == "orientation")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed Orientation '{}'.", pathStr, value));
      }
      xformOrientation = std::move(v.value());
    }
    else if(keyLower == "centerofrotation")
    {
      auto v = ParseFloatList(value);
      if(!v.has_value())
      {
        return MakeErrorResult<MetaImageMetadata>(-35810, fmt::format("MetaImage '{}' has malformed CenterOfRotation '{}'.", pathStr, value));
      }
      for(usize i = 0; i < 3 && i < v.value().size(); i++)
      {
        md.centerOfRotation[i] = static_cast<float32>(v.value()[i]);
      }
    }
    else if(keyLower == "elementdatafile")
    {
      elementDataFile = value;
      haveDataFile = true;
      // ElementDataFile is the LAST tag; the binary data (for LOCAL) begins at
      // the current stream position (just past this line's newline). A failed
      // position query returns std::streampos(-1); guard it so we don't wrap to
      // SIZE_MAX via the usize cast (a bogus offset the reader could not seek to).
      const std::streampos dataPos = in.tellg();
      if(dataPos == std::streampos(-1))
      {
        return MakeErrorResult<MetaImageMetadata>(-35813,
                                                  fmt::format("MetaImage '{}': failed to determine the data start offset after the 'ElementDataFile' tag (stream position query failed).", pathStr));
      }
      md.dataStartOffset = static_cast<usize>(dataPos);
      break;
    }
    // All other tags (CompressedDataSize, AnatomicalOrientation, Comment, ...) are ignored.
  }

  // ---- required fields ----
  if(!haveNDims || dimSize.empty() || metType.empty())
  {
    return MakeErrorResult<MetaImageMetadata>(-35803, fmt::format("MetaImage '{}' is missing one of the required fields: NDims / DimSize / ElementType.", pathStr));
  }
  if(!haveDataFile)
  {
    return MakeErrorResult<MetaImageMetadata>(-35809, fmt::format("MetaImage '{}' has no 'ElementDataFile' tag (header never terminated).", pathStr));
  }
  if(!binaryData)
  {
    return MakeErrorResult<MetaImageMetadata>(-35811, fmt::format("MetaImage '{}' has 'BinaryData = False'; ASCII pixel data is not supported.", pathStr));
  }
  if(nDims != 2 && nDims != 3)
  {
    return MakeErrorResult<MetaImageMetadata>(-35805, fmt::format("MetaImage '{}' has NDims {}; only 2D and 3D images are supported.", pathStr, nDims));
  }
  if(dimSize.size() != nDims)
  {
    return MakeErrorResult<MetaImageMetadata>(-35806, fmt::format("MetaImage '{}': DimSize count ({}) does not match NDims ({}).", pathStr, dimSize.size(), nDims));
  }
  for(usize d : dimSize)
  {
    if(d == 0)
    {
      return MakeErrorResult<MetaImageMetadata>(-35812, fmt::format("MetaImage '{}' has a zero-length axis in DimSize.", pathStr));
    }
  }
  const auto typeInfo = MetTypeToSimplnx(metType);
  if(!typeInfo.has_value())
  {
    return MakeErrorResult<MetaImageMetadata>(-35804, fmt::format("MetaImage '{}' has unsupported ElementType '{}'.", pathStr, metType));
  }

  md.nDims = nDims;
  md.dataType = typeInfo->first;
  md.elementSize = typeInfo->second;
  md.typeString = metType;
  md.encoding = compressed ? Encoding::Compressed : Encoding::Raw;

  // ---- resolve multi-alias fields by MetaIO's fixed precedence (independent of the
  //      order they appeared in the file), matching the last-read-wins behavior of
  //      metaObject.cxx / metaImage.cxx. ----
  // Origin: ImagePosition > Origin > Offset > Position.
  std::vector<float64> offset;
  if(offsetImagePosition.has_value())
  {
    offset = std::move(*offsetImagePosition);
  }
  else if(offsetOrigin.has_value())
  {
    offset = std::move(*offsetOrigin);
  }
  else if(offsetOffset.has_value())
  {
    offset = std::move(*offsetOffset);
  }
  else if(offsetPosition.has_value())
  {
    offset = std::move(*offsetPosition);
  }
  // Transform: TransformMatrix > Rotation > Orientation.
  if(xformTransformMatrix.has_value())
  {
    md.transformMatrix = std::move(*xformTransformMatrix);
  }
  else if(xformRotation.has_value())
  {
    md.transformMatrix = std::move(*xformRotation);
  }
  else if(xformOrientation.has_value())
  {
    md.transformMatrix = std::move(*xformOrientation);
  }
  // Byte order: BinaryDataByteOrderMSB > ElementByteOrderMSB.
  bool msbSpecified = false;
  bool msb = false;
  if(msbBinaryDataByteOrder.has_value())
  {
    msb = *msbBinaryDataByteOrder;
    msbSpecified = true;
  }
  else if(msbElementByteOrder.has_value())
  {
    msb = *msbElementByteOrder;
    msbSpecified = true;
  }

  // ---- dimensions [X,Y,Z] (Z=1 for 2D) ----
  md.dimensions = {1, 1, 1};
  for(usize i = 0; i < nDims; i++)
  {
    md.dimensions[i] = dimSize[i];
  }

  // ---- spacing (ElementSpacing preferred, ElementSize fallback), default 1 ----
  const std::vector<float64>& spacingSrc = !elementSpacing.empty() ? elementSpacing : elementSizeFallback;
  md.spacing = {1.0f, 1.0f, 1.0f};
  for(usize i = 0; i < nDims && i < spacingSrc.size(); i++)
  {
    md.spacing[i] = (spacingSrc[i] != 0.0) ? static_cast<float32>(spacingSrc[i]) : 1.0f;
  }

  // ---- origin (resolved from ImagePosition / Origin / Offset / Position above), default 0 ----
  md.origin = {0.0f, 0.0f, 0.0f};
  for(usize i = 0; i < nDims && i < offset.size(); i++)
  {
    md.origin[i] = static_cast<float32>(offset[i]);
  }

  // ---- endian / byteswap (only meaningful for multi-byte types) ----
  // MetaImage defaults to the writing host's byte order when the MSB tag is
  // absent, so an absent tag means "no swap"; a present tag is honored.
  if(md.elementSize > 1 && msbSpecified)
  {
    const bool fileIsLittle = !msb;
    const bool hostIsLittle = (nx::core::checkEndian() == nx::core::endian::little);
    md.byteSwapRequired = (fileIsLittle != hostIsLittle);
  }

  // ---- attached (LOCAL) vs detached data ----
  const std::string edfTrim = Trim(elementDataFile);
  const std::string edfLower = ToLower(edfTrim);
  if(edfLower == "local")
  {
    // MetaIO accepts "Local"/"LOCAL"/"local" (metaImage.cxx ~1223); lower-casing then
    // comparing to "local" is a superset that also covers those three forms.
    md.detached = false;
    md.dataFilePath = filePath;
    // md.dataStartOffset was recorded at the ElementDataFile line above.
  }
  else if(edfTrim.rfind("LIST", 0) == 0 || edfTrim.find('%') != std::string::npos)
  {
    // MetaIO treats an ElementDataFile whose FIRST FOUR chars are "LIST" as a multi-file
    // list (metaImage.cxx ~1230: `"LIST" == name.substr(0,4)`), and a name containing '%'
    // as an sprintf multi-file template (~1715/1813). Match the first-4-char prefix test
    // (case-sensitive, as MetaIO is) so a literal "LISTfoo.raw" is correctly rejected as a
    // list rather than mis-opened as data. A single detached filename that merely CONTAINS
    // spaces (e.g. "my scan.raw") is valid -- MetaIO takes the whole line as one name -- so
    // the old whitespace-count heuristic that misclassified it as multi-file is dropped.
    return MakeErrorResult<MetaImageMetadata>(-35807, fmt::format("MetaImage '{}': ElementDataFile LIST / multi-file / '%d' forms are not supported.", pathStr));
  }
  else
  {
    const fs::path df = filePath.parent_path() / edfTrim;
    if(!fs::exists(df))
    {
      return MakeErrorResult<MetaImageMetadata>(-35808, fmt::format("MetaImage '{}': detached data file '{}' does not exist.", pathStr, df.string()));
    }
    md.detached = true;
    md.dataFilePath = df;
    md.dataStartOffset = 0;
  }

  return {md};
}

Result<std::unique_ptr<MetaImageDataReader>> MetaImageDataReader::Create(const MetaImageMetadata& md)
{
  auto reader = std::unique_ptr<MetaImageDataReader>(new MetaImageDataReader());
  reader->m_Encoding = md.encoding;
  reader->m_FilePath = md.dataFilePath.string();
  reader->m_Stream.open(md.dataFilePath, std::ios::binary);
  if(!reader->m_Stream.is_open())
  {
    return MakeErrorResult<std::unique_ptr<MetaImageDataReader>>(-35830, fmt::format("Could not open MetaImage data file for reading: '{}'", reader->m_FilePath));
  }
  reader->m_Stream.seekg(static_cast<std::streamoff>(md.dataStartOffset), std::ios::beg);
  if(!reader->m_Stream.good())
  {
    return MakeErrorResult<std::unique_ptr<MetaImageDataReader>>(-35830, fmt::format("Failed to seek to MetaImage data offset {} in '{}'", md.dataStartOffset, reader->m_FilePath));
  }
  if(md.encoding == Encoding::Compressed)
  {
    reader->m_CompBuf.resize(k_ZlibReadBufferSize);
    reader->m_Zs = {};
    // 15 window bits + 32 => zlib auto-detects a zlib OR gzip header. MetaImage
    // CompressedData is zlib-format; this init decodes it (and gzip) transparently.
    if(inflateInit2(&reader->m_Zs, 15 + 32) != Z_OK)
    {
      return MakeErrorResult<std::unique_ptr<MetaImageDataReader>>(-35832, fmt::format("Failed to initialize zlib inflate for '{}'", reader->m_FilePath));
    }
    reader->m_ZInit = true;
  }
  return {std::move(reader)};
}

MetaImageDataReader::~MetaImageDataReader()
{
  if(m_ZInit)
  {
    inflateEnd(&m_Zs);
    m_ZInit = false;
  }
}

Result<> MetaImageDataReader::readBytes(void* dst, usize nBytes)
{
  if(nBytes == 0)
  {
    return {};
  }
  if(m_Encoding == Encoding::Raw)
  {
    m_Stream.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(nBytes));
    if(static_cast<usize>(m_Stream.gcount()) != nBytes)
    {
      return MakeErrorResult(-35831, fmt::format("Short read from MetaImage data '{}': requested {} bytes, got {}", m_FilePath, nBytes, static_cast<usize>(m_Stream.gcount())));
    }
    return {};
  }

  // Compressed: inflate exactly nBytes, refilling the compressed input buffer as needed.
  m_Zs.next_out = reinterpret_cast<Bytef*>(dst);
  assert(nBytes <= static_cast<usize>(std::numeric_limits<uInt>::max()));
  m_Zs.avail_out = static_cast<uInt>(nBytes);
  while(m_Zs.avail_out > 0)
  {
    if(m_Zs.avail_in == 0 && !m_SrcEof)
    {
      m_Stream.read(reinterpret_cast<char*>(m_CompBuf.data()), static_cast<std::streamsize>(m_CompBuf.size()));
      const std::streamsize got = m_Stream.gcount();
      if(got <= 0)
      {
        m_SrcEof = true;
      }
      m_Zs.next_in = m_CompBuf.data();
      m_Zs.avail_in = static_cast<uInt>(std::max<std::streamsize>(got, 0));
    }
    const uLong beforeIn = m_Zs.total_in;
    const uLong beforeOut = m_Zs.total_out;
    const int zret = inflate(&m_Zs, Z_NO_FLUSH);
    if(zret == Z_STREAM_END)
    {
      break;
    }
    if(zret == Z_OK)
    {
      continue;
    }
    if(zret == Z_BUF_ERROR)
    {
      // Terminal: the source is exhausted and no buffered input remains, so we
      // cannot satisfy the request (the short-read check below reports it).
      if(m_SrcEof && m_Zs.avail_in == 0)
      {
        break;
      }
      // Otherwise the `continue` relies on zlib's invariant that a non-terminal
      // Z_BUF_ERROR implies the outer loop can still make progress (refill input /
      // drain output). Defensively guard a pathological or corrupt stream that
      // advanced neither total_in nor total_out yet is not at EOF, which would
      // otherwise spin this loop forever.
      if(m_Zs.total_in == beforeIn && m_Zs.total_out == beforeOut)
      {
        return MakeErrorResult(-35832, fmt::format("zlib inflate made no forward progress while reading '{}' (corrupt or truncated stream).", m_FilePath));
      }
      continue;
    }
    return MakeErrorResult(-35832, fmt::format("zlib inflate error ({}) while reading '{}': {}", zret, m_FilePath, (m_Zs.msg != nullptr ? m_Zs.msg : "unknown")));
  }
  if(m_Zs.avail_out > 0)
  {
    return MakeErrorResult(-35831, fmt::format("Short read from compressed MetaImage data '{}': {} of {} bytes were unavailable.", m_FilePath, static_cast<usize>(m_Zs.avail_out), nBytes));
  }
  return {};
}

} // namespace nx::core::mhd
