#pragma once

#include <H5Ipublic.h>
#include <H5Ppublic.h>
#include <hdf5.h>

/**
 * @brief This class is to help cleanup HDF5 resources that are allocated. This
 * table should be followed when possible:
 *
| Function that **returns** an ID                      | What it returns                  | Matching **close** function                                      |
| ---------------------------------------------------- | -------------------------------- | ---------------------------------------------------------------- |
| `H5Aopen`, `H5Acreate`                                | Attribute ID                     | `H5Aclose`                                                       |
| `H5Aget_space`                                       | Dataspace ID                     | `H5Sclose`                                                       |
| `H5Aget_type`                                        | Datatype ID                      | `H5Tclose`                                                       |
| `H5Dopen`, `H5Dcreate`                               | Dataset ID                       | `H5Dclose`                                                       |
| `H5Dget_space`                                       | Dataspace ID                     | `H5Sclose`                                                       |
| `H5Dget_type`                                        | Datatype ID                      | `H5Tclose`                                                       |
| `H5Fopen`, `H5Fcreate`                               | File ID                          | `H5Fclose`                                                       |
| `H5Gopen`, `H5Gcreate`                               | Group ID                         | `H5Gclose`                                                       |
| `H5Oopen`                                            | Object ID (dataset, group, etc.) | Depends on object type (e.g. `H5Dclose`, `H5Gclose`, `H5Tclose`) |
| `H5Screate`, `H5Scopy`, `H5Dget_space`              | Dataspace ID                     | `H5Sclose`                                                       |
| `H5Tcopy`, `H5Tcreate`, `H5Aget_type`, `H5Dget_type`  | Datatype ID                      | `H5Tclose`                                                       |
| `H5Pcreate`, `H5Pcopy`                               | Property list ID                 | `H5Pclose`                                                       |
| `H5Eget_current_stack`                               | Error stack ID                   | `H5Eclose_stack`                                                 |
| `H5Ropen_object`, `H5Ropen_attr`                     | Referenced object/attribute ID   | Appropriate close (`H5Dclose`, `H5Gclose`, `H5Aclose`, etc.)     |
*
*
 */

template <herr_t (*Closer)(hid_t)>
class H5ResourceSentinel
{
public:
  hid_t id; // exposed publicly

  // Construct empty
  H5ResourceSentinel()
  : id(-1)
  {
  }

  // Construct with an already-opened ID
  explicit H5ResourceSentinel(hid_t handle)
  : id(handle)
  {
  }

  // Movable, not copyable
  H5ResourceSentinel(const H5ResourceSentinel&) = delete;
  H5ResourceSentinel& operator=(const H5ResourceSentinel&) = delete;

  H5ResourceSentinel(H5ResourceSentinel&& other) noexcept
  : id(other.id)
  {
    other.id = -1;
  }

  H5ResourceSentinel& operator=(H5ResourceSentinel&& other) noexcept
  {
    if(this != &other)
    {
      close();
      id = other.id;
      other.id = -1;
    }
    return *this;
  }

  /**
   * @brief Destructor — automatically closes if valid
   */
  ~H5ResourceSentinel()
  {
    close();
  }

  /**
   * @brief Closes the underlying hid_t resource and sets the hid_t value to -1
   * @return
   */
  herr_t close()
  {
    herr_t err = 0;
    if(id > 0)
    { // Only close valid IDs
      err = Closer(id);
      id = -1;
    }
    return err;
  }

  /**
   * @brief Releases ownership of the hid_t resource
   * @return Returns the underlying hid_t value so that something else can release it.
   */
  hid_t release()
  {
    hid_t tmp = id;
    id = -1;
    return tmp;
  }

  /**
   *
   * @return Returns the underlying hid_t value
   */
  hid_t value() const
  {
    return id;
  }

  /**
   *
   * @return Returns if the underlying hid_t value is valid, id>0
   */
  bool valid() const
  {
    return id > 0;
  }

  /**
   *
   * @return Returns if the underlying hid_t value is invalid, id < 0
   */
  bool invalid() const
  {
    return id < 0;
  }

  /**
   * @brief Resets the underlying hid_t value to -1, but DOES NOT CLOSE the resource.
   */
  void reset()
  {
    id = -1;
  }
};

// Convenient type aliases for common HDF5 objects
using H5AttributeCloser = H5ResourceSentinel<H5Aclose>;
using H5DatasetCloser = H5ResourceSentinel<H5Dclose>;
using H5GroupCloser = H5ResourceSentinel<H5Gclose>;
using H5FileCloser = H5ResourceSentinel<H5Fclose>;
using H5DatatypeCloser = H5ResourceSentinel<H5Tclose>;
using H5DataspaceCloser = H5ResourceSentinel<H5Sclose>;
using H5PropListCloser = H5ResourceSentinel<H5Pclose>;
