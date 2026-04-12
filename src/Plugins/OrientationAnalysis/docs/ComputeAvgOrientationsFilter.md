# Compute Feature Average Orientations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** computes the average orientation of each **Feature** using one or more of three available averaging methods. Each method can be independently enabled, and their results are stored in separate output arrays.

### Method 1: Rodrigues Average (Original)

This is the original averaging algorithm. It determines the average orientation of each **Feature** by:

1. Gathering all **Elements** that belong to the **Feature**
2. Using the symmetry operators of the phase of the **Feature**, rotating the quaternion of the **Feature**'s first **Element** into the *Fundamental Zone* nearest to the origin
3. Rotating each subsequent **Element**'s quaternion (with same symmetry operators) looking for the quaternion closest to the current running average
4. Accumulating a running sum of the nearest quaternions
5. Dividing the accumulated quaternion sum by the count and normalizing to produce the average

The process of finding the nearest quaternion in Step 3 accounts for the periodicity of orientation space, which would cause problems in the averaging if all quaternions were forced to be rotated into the same *Fundamental Zone*. The quaternions can be averaged with a simple summation-based average because quaternion space is not distorted like Euler space.

**Outputs:** Average Quaternions, Average Euler Angles (Bunge convention Z-X-Z)

### Method 2: Von Mises-Fisher (vMF) Average

The von Mises-Fisher distribution is a probability distribution on the surface of a unit hypersphere in *p*-dimensional space. For orientation averaging, the relevant case is the unit quaternion sphere (*p* = 4). The vMF distribution is parameterized by:

- **mu** (mean direction): A unit quaternion representing the central tendency of the distribution. This is the estimated average orientation.
- **kappa** (concentration parameter): A non-negative scalar that characterizes how tightly the orientations are clustered around the mean. Intuitively, kappa is to the vMF distribution what the full-width-at-half-maximum (FWHM) is to a Gaussian distribution: it is a measure of how narrow or tight the distribution is. Higher kappa values indicate tighter clustering (less spread); kappa = 0 corresponds to a uniform distribution on the sphere.

The vMF probability density for a unit vector **x** given mean direction **mu** and concentration **kappa** is proportional to exp(kappa * mu^T * x). This makes it the spherical analogue of the Gaussian distribution on a flat space.

The filter estimates the vMF parameters using an **Expectation-Maximization (EM)** algorithm. All element quaternions belonging to a feature are first reduced to the *Fundamental Zone* using the crystal symmetry operators. The EM procedure then iteratively refines the estimates of **mu** (the average orientation quaternion) and **kappa** (the concentration).

**Outputs:** Average Quaternions, Average Euler Angles (Bunge convention Z-X-Z), Kappa Values

### Method 3: Watson Average

The Watson distribution is a probability distribution on the unit sphere that is **antipodally symmetric**, meaning it treats **x** and **-x** as equivalent. This property makes it particularly well-suited for orientation data represented as quaternions, since quaternions **q** and **-q** represent the same physical rotation.

The Watson distribution is parameterized by:

- **mu** (mean axis): A unit quaternion representing the principal axis of the distribution. This is the estimated average orientation.
- **kappa** (concentration parameter): A scalar that controls the concentration of the distribution around the mean axis. As with the vMF distribution, kappa is analogous to the full-width-at-half-maximum (FWHM) of a Gaussian: it measures how narrow or tight the distribution is. For positive kappa, the distribution is bipolar (concentrated around +/-mu); for negative kappa, it is girdle-shaped (concentrated in the great circle perpendicular to mu).

The Watson probability density for a unit vector **x** is proportional to exp(kappa * (mu^T * x)^2). The key difference from the von Mises-Fisher distribution is the squared dot product, which enforces the antipodal symmetry.

Like the vMF method, the filter estimates Watson parameters using an **Expectation-Maximization (EM)** algorithm operating on fundamental-zone-reduced quaternions.

**Outputs:** Average Quaternions, Average Euler Angles (Bunge convention Z-X-Z), Kappa Values

### Hard-Coded Algorithm Parameters

The following parameters are currently hard-coded in the implementation and are not user-configurable:

| Parameter | Value | Description |
|-----------|-------|-------------|
| **Random Seed** | 43514 | Seed for the random number generator used in the EM algorithm. Because this is fixed, the vMF and Watson results are deterministic across runs. |
| **EM Iterations** | 5 | Number of Expectation-Maximization outer iterations. Controls how many times the full EM cycle is repeated. |
| **Iterations** | 10 | Number of inner iterations per EM cycle. Controls the refinement within each EM step. |

These values may be exposed as user-configurable parameters in a future release.

### Special Cases

- **Features with a single element:** For the vMF and Watson methods, if a feature contains only one element orientation, the EM algorithm is skipped entirely and the single quaternion is used directly as the average. The kappa value is set to 0 in this case.
- **Features with zero elements:** Features with no elements (phase <= 0 for all voxels) will have their output arrays initialized to NaN (for vMF/Watson) or identity quaternion / zero Euler angles (for Rodrigues).
- **Phase indexing:** The filter requires that phase values be > 0 for elements to be included in the averaging. Phase index 0 is reserved for "Unknown" in the Crystal Structures array and is always skipped.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) Small IN100 Full Reconstruction
+ InsertTransformationPhase
+ (05) SmallIN100 Crystallographic Statistics
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
