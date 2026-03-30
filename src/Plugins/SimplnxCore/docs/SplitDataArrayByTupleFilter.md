# Split Data Array (By Tuple)

## Group (Subgroup)

Data Manipulation (Memory/Management)

## Description

This **Filter** splits a **Data Array** into several smaller arrays along a single tuple dimension. 
The user specifies how many tuples each output array should contain along that dimension; all other dimensions remain unchanged.

For example, given an array with tuple shape `(4, 3)`:

```
{0} {1} {2}
{3} {4} {5}
{6} {7} {8}
{9} {10} {11}
```

with tuples for the output arrays set to 2 & 1 and split dimension set to 1 produces two new arrays with tuple shapes (4,2) and (4,1) respectively:

```
{0} {1}
{3} {4}
{6} {7}
{9} {10}
```

```
{2}
{5}
{8}
{11}
```


### Output Container

The *Output Container* parameter controls where the resulting split arrays are placed:

- **New Data Group**: Creates a new **DataGroup** to hold the split arrays.
- **Existing Data Group**: Places the split arrays into an existing **DataGroup** selected by the user.
- **New Attribute Matrix**: Creates a new **Attribute Matrix** to hold the split arrays.
- **Existing Attribute Matrix**: Places the split arrays into an existing **Attribute Matrix** selected by the user.

An optional flag allows you to delete the original input array after splitting.

**Looking to split by _components_ instead?**  

See the *[Split Data Array (By Component)](SplitDataArrayByComponentFilter.md)* filter that separates each component into its own scalar array.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D‑NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX‑Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D‑NX users can help answer your questions.