#include "NrrdUtilities.hpp"

#include "simplnx/Common/Bit.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cmath>
#include <limits>
#include <string_view>

namespace fs = std::filesystem;

namespace nx::core::nrrd
{
std::optional<std::pair<DataType, usize>> NrrdTypeToSimplnx(const std::string& nrrdType)
{
  // nrrdType is expected already trimmed + lower-cased + single-spaced.
  if(nrrdType == "signed char" || nrrdType == "int8" || nrrdType == "int8_t")
  {
    return std::make_pair(DataType::int8, usize{1});
  }
  if(nrrdType == "uchar" || nrrdType == "unsigned char" || nrrdType == "uint8" || nrrdType == "uint8_t")
  {
    return std::make_pair(DataType::uint8, usize{1});
  }
  if(nrrdType == "short" || nrrdType == "short int" || nrrdType == "signed short" || nrrdType == "signed short int" || nrrdType == "int16" || nrrdType == "int16_t")
  {
    return std::make_pair(DataType::int16, usize{2});
  }
  if(nrrdType == "ushort" || nrrdType == "unsigned short" || nrrdType == "unsigned short int" || nrrdType == "uint16" || nrrdType == "uint16_t")
  {
    return std::make_pair(DataType::uint16, usize{2});
  }
  if(nrrdType == "int" || nrrdType == "signed int" || nrrdType == "int32" || nrrdType == "int32_t")
  {
    return std::make_pair(DataType::int32, usize{4});
  }
  if(nrrdType == "uint" || nrrdType == "unsigned int" || nrrdType == "uint32" || nrrdType == "uint32_t")
  {
    return std::make_pair(DataType::uint32, usize{4});
  }
  if(nrrdType == "longlong" || nrrdType == "long long" || nrrdType == "long long int" || nrrdType == "signed long long" || nrrdType == "signed long long int" || nrrdType == "int64" ||
     nrrdType == "int64_t")
  {
    return std::make_pair(DataType::int64, usize{8});
  }
  if(nrrdType == "ulonglong" || nrrdType == "unsigned long long" || nrrdType == "unsigned long long int" || nrrdType == "uint64" || nrrdType == "uint64_t")
  {
    return std::make_pair(DataType::uint64, usize{8});
  }
  if(nrrdType == "float")
  {
    return std::make_pair(DataType::float32, usize{4});
  }
  if(nrrdType == "double")
  {
    return std::make_pair(DataType::float64, usize{8});
  }
  // "block" and everything else are unsupported.
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

// Collapse internal runs of whitespace to a single space (for multi-word type strings).
std::string NormalizeSpaces(const std::string& s)
{
  std::string out;
  out.reserve(s.size());
  bool inSpace = false;
  for(char c : s)
  {
    if(std::isspace(static_cast<unsigned char>(c)) != 0)
    {
      inSpace = true;
      continue;
    }
    if(inSpace && !out.empty())
    {
      out.push_back(' ');
    }
    inSpace = false;
    out.push_back(c);
  }
  return out;
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

// Split a "space directions" value into per-axis tokens: "none" or "(a,b,...)".
std::vector<std::string> SplitVectorTokens(const std::string& s)
{
  std::vector<std::string> out;
  usize i = 0;
  while(i < s.size())
  {
    if(std::isspace(static_cast<unsigned char>(s[i])) != 0)
    {
      i++;
      continue;
    }
    if(s[i] == '(')
    {
      const usize close = s.find(')', i);
      if(close == std::string::npos)
      {
        out.push_back(s.substr(i)); // malformed; caller will error on parse
        break;
      }
      out.push_back(s.substr(i, close - i + 1));
      i = close + 1;
    }
    else
    {
      const usize start = i;
      while(i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) == 0)
      {
        i++;
      }
      out.push_back(s.substr(start, i - start));
    }
  }
  return out;
}

std::optional<usize> ToUSize(const std::string& s)
{
  const std::string trimmed = Trim(s);
  // Reject a leading sign or any other non-digit lead before handing to stoull:
  // std::stoull silently ACCEPTS a leading '-' and wraps (e.g. "-1" -> ULLONG_MAX),
  // which would later overflow the allocation math into a bad_alloc. NRRD 'sizes'
  // and 'dimension' are strictly non-negative integers, so anything that does not
  // start with a digit is malformed.
  if(trimmed.empty() || std::isdigit(static_cast<unsigned char>(trimmed.front())) == 0)
  {
    return std::nullopt;
  }
  try
  {
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

// Parses a signed integer (for the 'byte skip' / 'line skip' fields, which teem
// stores as a long/unsigned int respectively). Returns nullopt if the whole
// trimmed token is not a single integer.
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

// Parse "(a,b,c)" (or a bare "a,b,c") into doubles. Returns nullopt on error.
std::optional<std::vector<float64>> ParseParenVector(const std::string& token)
{
  std::string inner = Trim(token);
  if(!inner.empty() && inner.front() == '(' && inner.back() == ')')
  {
    inner = inner.substr(1, inner.size() - 2);
  }
  std::vector<float64> out;
  std::string cur;
  auto flush = [&]() -> bool {
    const std::string t = Trim(cur);
    cur.clear();
    if(t.empty())
    {
      return false;
    }
    try
    {
      out.push_back(std::stod(t));
      return true;
    } catch(...)
    {
      return false;
    }
  };
  for(char c : inner)
  {
    if(c == ',')
    {
      if(!flush())
      {
        return std::nullopt;
      }
    }
    else
    {
      cur.push_back(c);
    }
  }
  if(!flush())
  {
    return std::nullopt;
  }
  return out;
}

// True if a "kinds" token denotes a non-spatial (component/list) axis.
// This is an ALLOWLIST of the NRRD kinds that genuinely denote a component/list
// axis. Everything else -- including "scalar", "time", "domain", "space", "stub",
// "none", "???" and the empty string -- is treated as a spatial axis. (A denylist
// would misclassify legitimate non-component kinds such as "scalar"/"time" as
// component axes.) The separate "space directions: none" signal still flags a
// component axis at the call site.
bool IsComponentKind(const std::string& kindLower)
{
  static constexpr std::array<std::string_view, 15> k_ComponentKinds = {"vector",           "2-vector", "3-vector",   "4-vector",  "list",       "point",     "2-point",  "3-point",
                                                                        "covariant-vector", "normal",   "quaternion", "rgb-color", "rgba-color", "hsv-color", "xyz-color"};
  return std::find(k_ComponentKinds.begin(), k_ComponentKinds.end(), kindLower) != k_ComponentKinds.end();
}

// Mirrors itkNrrdImageIO.cxx's on-read conversion of a named NRRD 'space' into ITK's
// LPS convention (ReadImageInformation, ITK 5.4: origin flip near lines 532-549). ITK
// sign-flips only the RAS and LAS spaces; every other space teem recognizes is left
// unchanged ("default:" case), and a space teem does NOT recognize is a hard error
// (teem's airEnumVal returns 0 -> "couldn't parse space"). Because this reader collapses
// 'space directions' to per-axis spacing magnitudes (sign-independent) and never flips
// voxel data (ITK's iFlipFactors only reorients the DTI measurement frame), the origin
// sign-flip is the sole visible effect. Matching is case-insensitive against teem's
// nrrdSpace string-equivalence table (NrrdIO/enumsNrrd.c: _nrrdSpaceStrEqv).
// @return {negateOriginX, negateOriginY} for a recognized space, or nullopt if
//         the value is not a space teem/ITK recognizes.
std::optional<std::pair<bool, bool>> NrrdSpaceOriginFlip(const std::string& spaceValue)
{
  const std::string s = NormalizeSpaces(ToLower(spaceValue));

  struct SpaceEntry
  {
    std::string_view name;
    bool negateX;
    bool negateY;
  };
  // Full teem equivalence set (NrrdIO/enumsNrrd.c). RAS -> negate X,Y; LAS -> negate X;
  // every other recognized space -> no change (matches ITK's switch default). Anything
  // not listed here is an unrecognized space and errors, exactly as teem does.
  static constexpr SpaceEntry k_Spaces[] = {
      // right-anterior-superior (RAS): R->L, A->P
      {"right-anterior-superior", true, true},
      {"right anterior superior", true, true},
      {"rightanteriorsuperior", true, true},
      {"ras", true, true},
      // left-anterior-superior (LAS): R->L
      {"left-anterior-superior", true, false},
      {"left anterior superior", true, false},
      {"leftanteriorsuperior", true, false},
      {"las", true, false},
      // left-posterior-superior (LPS): already ITK's target frame
      {"left-posterior-superior", false, false},
      {"left posterior superior", false, false},
      {"leftposteriorsuperior", false, false},
      {"lps", false, false},
      // *-time variants: ITK has no well-defined LPS conversion (default: no change)
      {"right-anterior-superior-time", false, false},
      {"right anterior superior time", false, false},
      {"rightanteriorsuperiortime", false, false},
      {"rast", false, false},
      {"left-anterior-superior-time", false, false},
      {"left anterior superior time", false, false},
      {"leftanteriorsuperiortime", false, false},
      {"last", false, false},
      {"left-posterior-superior-time", false, false},
      {"left posterior superior time", false, false},
      {"leftposteriorsuperiortime", false, false},
      {"lpst", false, false},
      // scanner-xyz / 3D-handed frames: no LPS conversion (default: no change)
      {"scanner-xyz", false, false},
      {"scanner-xyz-time", false, false},
      {"scanner-xyzt", false, false},
      {"3d-right-handed", false, false},
      {"3d right handed", false, false},
      {"3drighthanded", false, false},
      {"3d-left-handed", false, false},
      {"3d left handed", false, false},
      {"3dlefthanded", false, false},
      {"3d-right-handed-time", false, false},
      {"3d right handed time", false, false},
      {"3drighthandedtime", false, false},
      {"3d-left-handed-time", false, false},
      {"3d left handed time", false, false},
      {"3dlefthandedtime", false, false},
  };
  for(const auto& entry : k_Spaces)
  {
    if(s == entry.name)
    {
      return std::make_pair(entry.negateX, entry.negateY);
    }
  }
  return std::nullopt;
}
} // namespace

Result<NrrdMetadata> ReadNrrdHeader(const std::filesystem::path& filePath)
{
  const std::string pathStr = filePath.string();
  std::ifstream in(filePath, std::ios::binary);
  if(!in.is_open())
  {
    return MakeErrorResult<NrrdMetadata>(-35700, fmt::format("Could not open NRRD file for reading: '{}'", pathStr));
  }

  std::string line;
  if(!std::getline(in, line))
  {
    return MakeErrorResult<NrrdMetadata>(-35701, fmt::format("NRRD file '{}' is empty or unreadable.", pathStr));
  }
  line = StripCR(line);
  if(line.rfind("NRRD", 0) != 0)
  {
    return MakeErrorResult<NrrdMetadata>(-35702, fmt::format("File '{}' is not a NRRD file (first line '{}' does not begin with 'NRRD').", pathStr, line));
  }

  NrrdMetadata md;
  md.filePath = pathStr;
  md.version = (line.size() >= 8 && std::isdigit(static_cast<unsigned char>(line[7])) != 0) ? (line[7] - '0') : 0;

  std::string typeStr;
  std::string encodingStr;
  std::string endianStr;
  std::string spaceField;
  std::string dataFileField;
  usize dimension = 0;
  bool haveDimension = false;
  std::vector<usize> sizes;
  std::vector<std::string> kinds;
  std::vector<std::string> spaceDirTokens;
  std::vector<float64> spaceOrigin;
  std::vector<float64> spacings;

  bool headerTerminated = false;
  while(std::getline(in, line))
  {
    line = StripCR(line);
    if(line.empty())
    {
      md.dataStartOffset = static_cast<usize>(in.tellg());
      headerTerminated = true;
      break;
    }
    if(line[0] == '#')
    {
      continue;
    }
    const auto colon = line.find(':');
    if(colon == std::string::npos)
    {
      continue; // not a field line
    }
    const std::string keyLower = ToLower(Trim(line.substr(0, colon)));
    std::string value;
    if(colon + 1 < line.size() && line[colon + 1] == '=')
    {
      // key:=value key/value pair — not consumed by this reader.
      continue;
    }
    value = Trim(line.substr(colon + 1));

    if(keyLower == "type")
    {
      typeStr = NormalizeSpaces(ToLower(value));
    }
    else if(keyLower == "dimension")
    {
      auto d = ToUSize(value);
      if(!d.has_value())
      {
        return MakeErrorResult<NrrdMetadata>(-35706, fmt::format("NRRD '{}' has non-numeric dimension '{}'.", pathStr, value));
      }
      dimension = d.value();
      haveDimension = true;
    }
    else if(keyLower == "sizes")
    {
      for(const auto& tok : SplitWhitespace(value))
      {
        auto v = ToUSize(tok);
        if(!v.has_value())
        {
          return MakeErrorResult<NrrdMetadata>(-35706, fmt::format("NRRD '{}' has non-numeric size token '{}'.", pathStr, tok));
        }
        sizes.push_back(v.value());
      }
    }
    else if(keyLower == "encoding")
    {
      encodingStr = ToLower(value);
    }
    else if(keyLower == "endian")
    {
      endianStr = ToLower(value);
    }
    else if(keyLower == "kinds")
    {
      kinds = SplitWhitespace(value);
    }
    else if(keyLower == "space directions")
    {
      spaceDirTokens = SplitVectorTokens(value);
    }
    else if(keyLower == "space origin")
    {
      auto v = ParseParenVector(value);
      if(!v.has_value())
      {
        return MakeErrorResult<NrrdMetadata>(-35711, fmt::format("NRRD '{}' has malformed space origin '{}'.", pathStr, value));
      }
      spaceOrigin = v.value();
    }
    else if(keyLower == "spacings")
    {
      for(const auto& tok : SplitWhitespace(value))
      {
        try
        {
          spacings.push_back(std::stod(tok));
        } catch(...)
        {
          spacings.push_back(std::nan(""));
        }
      }
    }
    else if(keyLower == "data file" || keyLower == "datafile")
    {
      dataFileField = value;
    }
    else if(keyLower == "space")
    {
      spaceField = value;
    }
    else if(keyLower == "byte skip" || keyLower == "byteskip")
    {
      // teem/ITK honor 'byte skip' (skip this many bytes before the data segment; a
      // negative value skips backward from EOF). This reader does not skip, so a nonzero
      // value would mean reading the wrong bytes -> reject loudly instead of silently
      // producing wrong voxels. 'byte skip: 0' is a no-op and is accepted. Mirrors the
      // 'data file: LIST' rejection (-35709): fail, don't corrupt.
      const auto v = ToLong(value);
      if(!v.has_value() || v.value() != 0)
      {
        return MakeErrorResult<NrrdMetadata>(
            -35714, fmt::format("NRRD '{}' specifies an unsupported 'byte skip' of '{}'. This reader cannot skip bytes before the data segment (teem/ITK honor it), so it refuses rather than read the "
                                "wrong voxels.",
                                pathStr, value));
      }
    }
    else if(keyLower == "line skip" || keyLower == "lineskip")
    {
      // Same rationale as 'byte skip' above: honoring it is unimplemented, so a nonzero
      // 'line skip' is rejected rather than silently ignored. 'line skip: 0' is accepted.
      const auto v = ToLong(value);
      if(!v.has_value() || v.value() != 0)
      {
        return MakeErrorResult<NrrdMetadata>(
            -35715, fmt::format("NRRD '{}' specifies an unsupported 'line skip' of '{}'. This reader cannot skip lines before the data segment (teem/ITK honor it), so it refuses rather than read the "
                                "wrong voxels.",
                                pathStr, value));
      }
    }
    // All other keys (content, etc.) are ignored.
  }

  // ---- required fields ----
  if(typeStr.empty() || encodingStr.empty() || !haveDimension || sizes.empty())
  {
    return MakeErrorResult<NrrdMetadata>(-35703, fmt::format("NRRD '{}' is missing one of the required fields: type / dimension / sizes / encoding.", pathStr));
  }
  const auto typeInfo = NrrdTypeToSimplnx(typeStr);
  if(!typeInfo.has_value())
  {
    return MakeErrorResult<NrrdMetadata>(-35704, fmt::format("NRRD '{}' has unsupported type '{}'. Supported: (unsigned) char/short/int/long long, float, double.", pathStr, typeStr));
  }
  if(encodingStr == "raw")
  {
    md.encoding = Encoding::Raw;
  }
  else if(encodingStr == "gzip" || encodingStr == "gz")
  {
    md.encoding = Encoding::Gzip;
  }
  else
  {
    return MakeErrorResult<NrrdMetadata>(-35705, fmt::format("NRRD '{}' uses unsupported encoding '{}'. Supported: raw, gzip.", pathStr, encodingStr));
  }
  if(sizes.size() != dimension)
  {
    return MakeErrorResult<NrrdMetadata>(-35706, fmt::format("NRRD '{}': sizes count ({}) does not match dimension ({}).", pathStr, sizes.size(), dimension));
  }
  for(usize s : sizes)
  {
    if(s == 0)
    {
      return MakeErrorResult<NrrdMetadata>(-35712, fmt::format("NRRD '{}' has a zero-length axis in sizes.", pathStr));
    }
  }

  md.dataType = typeInfo->first;
  md.elementSize = typeInfo->second;
  md.typeString = typeStr;
  md.rawSizes = sizes;

  // ---- component (interleaved) axis detection: must be the fastest axis (axis 0) ----
  usize componentAxis = std::numeric_limits<usize>::max();
  for(usize i = 0; i < dimension; i++)
  {
    bool isComp = false;
    if(i < kinds.size() && IsComponentKind(ToLower(kinds[i])))
    {
      isComp = true;
    }
    if(i < spaceDirTokens.size() && ToLower(spaceDirTokens[i]) == "none")
    {
      isComp = true;
    }
    if(isComp)
    {
      componentAxis = i;
      break;
    }
  }
  if(componentAxis != std::numeric_limits<usize>::max() && componentAxis != 0)
  {
    return MakeErrorResult<NrrdMetadata>(
        -35707, fmt::format("NRRD '{}': the component/vector axis (index {}) must be the fastest axis (index 0) to map to interleaved simplnx components.", pathStr, componentAxis));
  }
  const bool hasComponentAxis = (componentAxis == 0);
  md.componentCount = hasComponentAxis ? sizes[0] : 1;
  const usize spatialStart = hasComponentAxis ? 1 : 0;
  md.spatialDim = dimension - spatialStart;
  if(md.spatialDim != 2 && md.spatialDim != 3)
  {
    return MakeErrorResult<NrrdMetadata>(-35708, fmt::format("NRRD '{}': spatial dimension {} is unsupported (only 2D and 3D images are read).", pathStr, md.spatialDim));
  }

  // ---- dimensions [X,Y,Z] (Z=1 for 2D) ----
  md.dimensions = {1, 1, 1};
  for(usize s = 0; s < md.spatialDim; s++)
  {
    md.dimensions[s] = sizes[spatialStart + s];
  }

  // ---- spacing + rotation from space directions (preferred) or spacings ----
  md.spacing = {1.0f, 1.0f, 1.0f};
  if(!spaceDirTokens.empty())
  {
    for(usize s = 0; s < md.spatialDim; s++)
    {
      const usize axis = spatialStart + s;
      if(axis >= spaceDirTokens.size())
      {
        break;
      }
      const std::string& tok = spaceDirTokens[axis];
      if(ToLower(tok) == "none")
      {
        md.spacing[s] = 1.0f;
        continue;
      }
      auto v = ParseParenVector(tok);
      if(!v.has_value())
      {
        return MakeErrorResult<NrrdMetadata>(-35711, fmt::format("NRRD '{}' has malformed space direction '{}'.", pathStr, tok));
      }
      const auto& vec = v.value();
      float64 sumSq = 0.0;
      for(float64 c : vec)
      {
        sumSq += c * c;
      }
      const float64 mag = std::sqrt(sumSq);
      md.spacing[s] = (mag > 0.0) ? static_cast<float32>(mag) : 1.0f;
      // Axis-aligned means vec has one non-zero component, at index s.
      for(usize c = 0; c < vec.size(); c++)
      {
        if(c == s)
        {
          continue;
        }
        if(std::fabs(vec[c]) > 1.0e-6 * std::max(mag, 1.0e-6))
        {
          md.affineHasRotation = true;
        }
      }
    }
  }
  else if(!spacings.empty())
  {
    for(usize s = 0; s < md.spatialDim; s++)
    {
      const usize axis = spatialStart + s;
      if(axis >= spacings.size())
      {
        break;
      }
      const float64 sp = spacings[axis];
      md.spacing[s] = (std::isnan(sp) || sp == 0.0) ? 1.0f : static_cast<float32>(sp);
    }
  }

  // ---- origin ----
  md.origin = {0.0f, 0.0f, 0.0f};
  for(usize s = 0; s < md.spatialDim && s < spaceOrigin.size(); s++)
  {
    md.origin[s] = static_cast<float32>(spaceOrigin[s]);
  }

  // ---- named 'space' -> LPS origin sign-flip (parity with itkNrrdImageIO.cxx) ----
  // A bare numeric 'space dimension' (no named space) leaves spaceField empty and is a
  // no-op here, preserving the existing 2-D behavior.
  if(!spaceField.empty())
  {
    const auto flip = NrrdSpaceOriginFlip(spaceField);
    if(!flip.has_value())
    {
      return MakeErrorResult<NrrdMetadata>(
          -35716, fmt::format("NRRD '{}' has an unrecognized 'space' value '{}'. teem/ITK's NrrdIO rejects a 'space' it cannot map to a known coordinate frame.", pathStr, spaceField));
    }
    if(flip->first)
    {
      md.origin[0] = -md.origin[0];
    }
    if(flip->second)
    {
      md.origin[1] = -md.origin[1];
    }
  }

  // ---- endian / byteswap (only meaningful for multi-byte types) ----
  // NRRD requires an 'endian' field for any multi-byte type stored with an
  // endian-sensitive encoding (raw and gzip both are). teem/ITK's NrrdIO ERRORS in
  // this case ("type (...) and encoding (...) require endian info") rather than
  // assuming a default byte order, so we match that behavior exactly instead of
  // silently assuming host order (which would be a silent ITK divergence).
  if(md.elementSize > 1)
  {
    if(endianStr.empty())
    {
      return MakeErrorResult<NrrdMetadata>(
          -35713, fmt::format("NRRD '{}' stores a multi-byte type ('{}') with '{}' encoding but is missing the required 'endian' field (expected 'little' or 'big').", pathStr, typeStr, encodingStr));
    }
    const bool fileIsLittle = (endianStr == "little");
    const bool hostIsLittle = (nx::core::checkEndian() == nx::core::endian::little);
    md.byteSwapRequired = (fileIsLittle != hostIsLittle);
  }

  // ---- attached vs detached data ----
  if(!dataFileField.empty())
  {
    const std::string dfLower = ToLower(Trim(dataFileField));
    // Only the actual multi-file forms are rejected: the 'LIST' keyword and the
    // sprintf ('%d') filename template. A single detached filename that merely
    // contains spaces (e.g. "data file: my scan.raw") is valid -- teem takes the
    // whole remainder of the line as one filename -- so it must NOT be misread as
    // a multi-file spec (the previous whitespace-count check did exactly that).
    if(dfLower == "list" || dfLower.rfind("list ", 0) == 0 || dataFileField.find('%') != std::string::npos)
    {
      return MakeErrorResult<NrrdMetadata>(-35709, fmt::format("NRRD '{}': detached 'data file' LIST / multi-file / sprintf forms are not supported.", pathStr));
    }
    const fs::path df = filePath.parent_path() / Trim(dataFileField);
    if(!fs::exists(df))
    {
      return MakeErrorResult<NrrdMetadata>(-35710, fmt::format("NRRD '{}': detached data file '{}' does not exist.", pathStr, df.string()));
    }
    md.detached = true;
    md.dataFilePath = df;
    md.dataStartOffset = 0;
  }
  else
  {
    if(!headerTerminated)
    {
      return MakeErrorResult<NrrdMetadata>(-35703, fmt::format("NRRD '{}' has no blank line terminating the header and no detached data file.", pathStr));
    }
    md.detached = false;
    md.dataFilePath = filePath;
  }

  return {md};
}

Result<std::unique_ptr<NrrdDataReader>> NrrdDataReader::Create(const NrrdMetadata& md)
{
  auto reader = std::unique_ptr<NrrdDataReader>(new NrrdDataReader());
  reader->m_Encoding = md.encoding;
  reader->m_FilePath = md.dataFilePath.string();
  reader->m_Stream.open(md.dataFilePath, std::ios::binary);
  if(!reader->m_Stream.is_open())
  {
    return MakeErrorResult<std::unique_ptr<NrrdDataReader>>(-35730, fmt::format("Could not open NRRD data file for reading: '{}'", reader->m_FilePath));
  }
  reader->m_Stream.seekg(static_cast<std::streamoff>(md.dataStartOffset), std::ios::beg);
  if(!reader->m_Stream.good())
  {
    return MakeErrorResult<std::unique_ptr<NrrdDataReader>>(-35730, fmt::format("Failed to seek to NRRD data offset {} in '{}'", md.dataStartOffset, reader->m_FilePath));
  }
  if(md.encoding == Encoding::Gzip)
  {
    reader->m_CompBuf.resize(k_GzReadBufferSize);
    reader->m_Zs = {};
    // 15 window bits + 32 => zlib auto-detects a gzip OR zlib header.
    if(inflateInit2(&reader->m_Zs, 15 + 32) != Z_OK)
    {
      return MakeErrorResult<std::unique_ptr<NrrdDataReader>>(-35732, fmt::format("Failed to initialize zlib inflate for '{}'", reader->m_FilePath));
    }
    reader->m_ZInit = true;
  }
  return {std::move(reader)};
}

NrrdDataReader::~NrrdDataReader()
{
  if(m_ZInit)
  {
    inflateEnd(&m_Zs);
    m_ZInit = false;
  }
}

Result<> NrrdDataReader::readBytes(void* dst, usize nBytes)
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
      return MakeErrorResult(-35731, fmt::format("Short read from NRRD data '{}': requested {} bytes, got {}", m_FilePath, nBytes, static_cast<usize>(m_Stream.gcount())));
    }
    return {};
  }

  // Gzip: inflate exactly nBytes, refilling the compressed input buffer as needed.
  m_Zs.next_out = reinterpret_cast<Bytef*>(dst);
  // A single request is one scan-line (or a whole small array in tests); a >4 GiB
  // scan-line is effectively impossible, but make the narrowing to uInt explicit.
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
        return MakeErrorResult(-35732, fmt::format("zlib inflate made no forward progress while reading '{}' (corrupt or truncated gzip stream).", m_FilePath));
      }
      continue;
    }
    return MakeErrorResult(-35732, fmt::format("zlib inflate error ({}) while reading '{}': {}", zret, m_FilePath, (m_Zs.msg != nullptr ? m_Zs.msg : "unknown")));
  }
  if(m_Zs.avail_out > 0)
  {
    return MakeErrorResult(-35731, fmt::format("Short read from gzip NRRD data '{}': {} of {} bytes were unavailable.", m_FilePath, static_cast<usize>(m_Zs.avail_out), nBytes));
  }
  return {};
}

} // namespace nx::core::nrrd
