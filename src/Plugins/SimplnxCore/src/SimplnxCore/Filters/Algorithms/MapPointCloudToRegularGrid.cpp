#include "MapPointCloudToRegularGrid.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr nx::core::StringLiteral k_SilentMode = "Silent";
constexpr nx::core::StringLiteral k_WarningMode = "Warning with Count";
constexpr nx::core::StringLiteral k_ErrorMode = "Error at First Instance";
const nx::core::ChoicesParameter::ValueType k_SilentModeIndex = 0;
const nx::core::ChoicesParameter::ValueType k_WarningModeIndex = 1;
const nx::core::ChoicesParameter::ValueType k_ErrorModeIndex = 2;

constexpr int64 k_MaskCompareInvalid = -2605;
constexpr int64 k_ErrorOutOfBounds = -2607;
constexpr int64 k_WarningOutOfBounds = -2608;
constexpr int64 k_InvalidHandlingValue = -2609;

template <bool UseSilent, bool UseWarning, bool UseError>
struct OutOfBoundsType
{
  // Compile time checks for bounding, no runtime overhead
  static_assert((UseSilent && !UseWarning && !UseError) || (!UseSilent && UseWarning && !UseError) || (!UseSilent && !UseWarning && UseError),
                "struct `OutOfBoundsType` can only have one true bool in its instantiation");

  static constexpr bool UsingSilent = UseSilent;
  static constexpr bool UsingWarning = UseWarning;
  static constexpr bool UsingError = UseError;
};

using SilentType = OutOfBoundsType<true, false, false>;
using WarningType = OutOfBoundsType<false, true, false>;
using ErrorType = OutOfBoundsType<false, false, true>;

template <class OutOfBoundsType = SilentType, bool UseMask = false>
Result<> ProcessVertices(const IFilter::MessageHandler& messageHandler, const VertexGeom& vertices, const ImageGeom& image, UInt64AbstractDataStore& voxelIndices,
                         const std::unique_ptr<MaskCompareUtilities::MaskCompare>& maskCompare, uint64 outOfBoundsValue)
{
  // Out of Bounds Counter
  usize count = 0;

  // Execution
  usize numVerts = vertices.getNumberOfVertices();
  auto start = std::chrono::steady_clock::now();
  for(int64 i = 0; i < numVerts; i++)
  {
    if constexpr(UseMask)
    {
      if(!maskCompare->isTrue(i))
      {
        continue;
      }
    }

    auto coords = vertices.getVertexCoordinate(i);
    const auto indexResult = image.getIndex(coords[0], coords[1], coords[2]);
    if(indexResult.has_value())
    {
      voxelIndices[i] = indexResult.value();
    }
    else
    {
      if constexpr(OutOfBoundsType::UsingError)
      {
        BoundingBox3Df imageBounds = image.getBoundingBoxf();
        const Point3Df& minPoint = imageBounds.getMinPoint();
        const Point3Df& maxPoint = imageBounds.getMaxPoint();
        return MakeErrorResult(
            k_ErrorOutOfBounds,
            fmt::format("Out of bounds value encountered.\nVertex Index: {}\nVertex Coordinates [X,Y,Z]: [{},{},{}]\nImage Coordinate Bounds:\nX: {} to {}\nY: {} to {}\nZ: {} to {}", i, coords[0],
                        coords[1], coords[2], minPoint.getX(), maxPoint.getX(), minPoint.getY(), maxPoint.getY(), minPoint.getZ(), maxPoint.getZ()));
      }

      // Out of bounds value
      voxelIndices[i] = outOfBoundsValue;
      count++;
    }

    auto now = std::chrono::steady_clock::now();
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      messageHandler.sendInfoMessage(fmt::format("Computing Point Cloud Voxel Indices || {}% Completed", static_cast<int64>((static_cast<float32>(i) / numVerts) * 100.0f)));
      start = now;
    }
  }

  if constexpr(OutOfBoundsType::UsingWarning)
  {
    if(count > 0)
    {
      return MakeWarningVoidResult(k_WarningOutOfBounds, fmt::format("Mapping Complete. Number of value outside image bounds: {}", count));
    }
  }

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
MapPointCloudToRegularGrid::MapPointCloudToRegularGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       MapPointCloudToRegularGridInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
MapPointCloudToRegularGrid::~MapPointCloudToRegularGrid() noexcept = default;


// -----------------------------------------------------------------------------
const std::atomic_bool& MapPointCloudToRegularGrid::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> MapPointCloudToRegularGrid::operator()()
{
  // Get the target image as a pointer
  const auto& image = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);

  // Create the Mask
  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(k_MaskCompareInvalid, message);
  }

  // Cache all the needed objects for ::ProcessVertices
  const auto& vertices = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->VertexGeomPath);
  auto& voxelIndices = m_DataStructure.getDataAs<UInt64Array>(m_InputValues->VoxelIndicesPath)->getDataStoreRef();

  // Execute the correct ::ProcessVertices, else error out
  if(m_InputValues->UseMask)
  {
    switch(m_InputValues->OutOfBoundsHandling)
    {
    case k_SilentModeIndex: {
      return ProcessVertices<SilentType, true>(m_MessageHandler, vertices, image, voxelIndices, maskCompare, m_InputValues->OutOfBoundsValue);
    }
    case k_WarningModeIndex: {
      return ProcessVertices<WarningType, true>(m_MessageHandler, vertices, image, voxelIndices, maskCompare, m_InputValues->OutOfBoundsValue);
    }
    case k_ErrorModeIndex: {
      return ProcessVertices<ErrorType, true>(m_MessageHandler, vertices, image, voxelIndices, maskCompare, m_InputValues->OutOfBoundsValue);
    }
    default: {
      return MakeErrorResult(k_InvalidHandlingValue, fmt::format("Unexpected Out of Bounds Handling Option. Received : {}. Expected: {} ({}), {} ({}), {} ({})", m_InputValues->OutOfBoundsHandling,
                                                                 k_SilentMode, k_SilentModeIndex, k_WarningMode, k_WarningModeIndex, k_ErrorMode, k_ErrorModeIndex));
    }
    }
  }
  else
  {
    switch(m_InputValues->OutOfBoundsHandling)
    {
    case k_SilentModeIndex: {
      return ProcessVertices<SilentType, false>(m_MessageHandler, vertices, image, voxelIndices, maskCompare, m_InputValues->OutOfBoundsValue);
    }
    case k_WarningModeIndex: {
      return ProcessVertices<WarningType, false>(m_MessageHandler, vertices, image, voxelIndices, maskCompare, m_InputValues->OutOfBoundsValue);
    }
    case k_ErrorModeIndex: {
      return ProcessVertices<ErrorType, false>(m_MessageHandler, vertices, image, voxelIndices, maskCompare, m_InputValues->OutOfBoundsValue);
    }
    default: {
      return MakeErrorResult(k_InvalidHandlingValue, fmt::format("Unexpected Out of Bounds Handling Option. Received : {}. Expected: {} ({}), {} ({}), {} ({})", m_InputValues->OutOfBoundsHandling,
                                                                 k_SilentMode, k_SilentModeIndex, k_WarningMode, k_WarningModeIndex, k_ErrorMode, k_ErrorModeIndex));
    }
    }
  }
}
