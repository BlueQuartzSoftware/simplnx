#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <H5Ipublic.h>

namespace nx::core::HDF5
{

/**
 * @brief Tests the host byte order at runtime.
 * @return True when the host stores the least-significant byte first.
 */
SIMPLNX_EXPORT bool hostIsLittleEndian();

/**
 * @brief Tests whether raw deflate chunk I/O can bypass the HDF5 filter pipeline.
 * @param datasetId Identifies an open HDF5 dataset.
 * @param elementSize Specifies bytes in one dataset element.
 * @param deflateLevelOut Receives the dataset deflate level when available, or is null.
 * @return True when the raw parallel deflate paths are eligible.
 * @pre datasetId is valid and elementSize is nonzero.
 * @pre The caller does not hold Support::ApiLock().
 *
 * Eligibility requires exactly one deflate filter. It also requires matching file
 * and host byte order unless elements have one byte. Other filter pipelines use
 * HDF5 so it can apply filters and byte conversion correctly. This includes
 * shuffle, SZIP, Fletcher32, multiple-filter, and unknown pipelines.
 *
 * The loader and writer share this probe, so their eligibility rules stay equal.
 * The function gets Support::ApiLock() for all HDF5 calls. The lock is not recursive.
 * If the pipeline is not single-filter deflate, deflateLevelOut stays unchanged.
 * Eligible reads use positional raw I/O and inflate. Eligible writes use compression
 * followed by H5Dwrite_chunk.
 */
SIMPLNX_EXPORT bool probeSingleDeflateEligibility(hid_t datasetId, usize elementSize, int32* deflateLevelOut);

} // namespace nx::core::HDF5
