#include "NeighborListIO.hpp"

namespace complex::HDF5
{
template class COMPLEX_EXPORT NeighborListIO<int8>;
template class COMPLEX_EXPORT NeighborListIO<uint8>;

template class COMPLEX_EXPORT NeighborListIO<int16>;
template class COMPLEX_EXPORT NeighborListIO<uint16>;

template class COMPLEX_EXPORT NeighborListIO<int32>;
template class COMPLEX_EXPORT NeighborListIO<uint32>;

template class COMPLEX_EXPORT NeighborListIO<int64>;
template class COMPLEX_EXPORT NeighborListIO<uint64>;

template class COMPLEX_EXPORT NeighborListIO<float32>;
template class COMPLEX_EXPORT NeighborListIO<float64>;
} // namespace complex::HDF5
