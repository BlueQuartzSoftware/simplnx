# Split Data Array (By Tuple)

## Group (Subgroup)

DREAM3D Review (Memory/Management)

## Description

This **Filter** splits an **Attribute Array** into **several smaller arrays based on the *tuple* layout**.

The user tells the filter how many tuples should go into each output array by specifying one or more **tuple‑shape blocks** (for example `5 | 2 | 3`).

Each output array is an ***exact copy* of the selected tuples (all components are preserved unchanged)** and therefore has the **same primitive type** and **same component‑count** as the original array.

Using a 1‑component, (4, 3) tuple shape integer array as an example:

```
{0} {1} {2} {3}
{4} {5} {6} {7}
{8} {9} {10} {11}
```

A tuple‑shape list of `(2, 1) and (1, 1)` produces two new arrays:

```
{0} {1}                     (2 tuples)
{3}                         (1 tuple)
```

If you choose *Existing Data Group / Attribute Matrix* the split arrays are placed into a pre‑existing container.  
Otherwise the filter can create a *new* Data Group or *new* Attribute Matrix to hold the results.  
An optional flag allows you to **delete the original input array** after splitting.

> **Looking to split by _components_ instead?**  
> See the *[Split Data Array (By Component)](SplitDataArrayByComponentFilter.md)* filter that separates each component into its own scalar array.

% Auto generated parameter table will be inserted here

## Example Pipelines

* Split a 3‑component `UInt8` color array into training / validation / test sets of 60 % / 20 % / 20 % tuples.  
* Divide a very large `NeighborList` into smaller chunks so each GPU kernel gets a contiguous range of tuples.

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D‑NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX‑Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D‑NX users can help answer your questions.