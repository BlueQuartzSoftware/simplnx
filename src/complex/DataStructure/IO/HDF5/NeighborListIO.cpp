#include "NeighborListIO.hpp"

namespace complex::HDF5
{
template class NeighborListIO<int8>;
template class NeighborListIO<uint8>;

template class NeighborListIO<int16>;
template class NeighborListIO<uint16>;

template class NeighborListIO<int32>;
template class NeighborListIO<uint32>;

template class NeighborListIO<int64>;
template class NeighborListIO<uint64>;

template class NeighborListIO<float32>;
template class NeighborListIO<float64>;
} // namespace complex::HDF5
