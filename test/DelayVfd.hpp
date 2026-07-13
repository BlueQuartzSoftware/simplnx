#pragma once

#include "simplnx/Common/Types.hpp"

#include <H5Ipublic.h>

/**
 * @brief A read-only HDF5 Virtual File Driver that adds a configurable sleep to
 * every file open and read.
 *
 * Why: the cost of preflighting a .dream3d file on network storage is
 * per-operation latency multiplied by the hundreds of small metadata reads an
 * HDF5 tree traversal performs. Injecting that latency at the VFD layer
 * reproduces the pathology deterministically on a local disk -- no root
 * privileges, no network, identical on every platform -- so benchmarks can
 * demonstrate the before/after effect of the preflight metadata cache.
 */
namespace DelayVfd
{
/**
 * @brief Registers the driver with HDF5 (idempotent) and returns its id for use
 * with H5Pset_driver. Returns H5I_INVALID_HID if registration fails.
 * @return The registered HDF5 driver id, or H5I_INVALID_HID on failure.
 */
hid_t Register();

/**
 * @brief Sets the sleep applied to each open and read call.
 * @param microseconds Per-operation delay in microseconds; 0 disarms the driver
 * so it behaves as a plain pass-through.
 */
void SetDelayMicroseconds(nx::core::uint64 microseconds);
} // namespace DelayVfd
