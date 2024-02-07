# Los Alamos FFT Writer

## Group (Subgroup)

IO (Output)

## Description

This **Filter** will create the directories along the path to the file if possible.

This **Filter** writes out **CellData** from an **Image Geometry** to a file that is appropriate to use as an input into Ricardo Lebensohn's FFT 3D simulation codes<sup>[1]</sup>.

The format of the file is an ASCII text file with the following space delimited information:

    Phi1   Phi   Phi2  X  Y   Z  Feature_ID   Phase_ID

The Euler angles are in degrees. X, Y, Z are integer indices into the **Image** geometry. Feature ID & Phase ID are the integer values for the feature and phase **Starting at One (1)**.

### Example Output

The output file:

    90.000 0.000 0.000 1 1 1 0 1
    90.000 0.000 0.000 2 1 1 0 1 
    90.000 0.000 0.000 3 1 1 0 1  
    90.000 0.000 0.000 4 1 1 0 1
    90.000 0.000 0.000 5 1 1 0 1  
    90.000 0.000 0.000 6 1 1 0 1
    90.000 0.000 0.000 7 1 1 0 1
    90.000 0.000 0.000 8 1 1 0 1
    90.000 0.000 0.000 9 1 1 0 1  
    90.000 0.000 0.000 10 1 1 0 1
    90.000 0.000 0.000 11 1 1 0 1 
    90.000 0.000 0.000 12 1 1 0 1  
    90.000 0.000 0.000 13 1 1 0 1
    90.000 0.000 0.000 14 1 1 0 1  
    90.000 0.000 0.000 15 1 1 0 1
    90.000 0.000 0.000 16 1 1 0 1
    90.000 0.000 0.000 17 1 1 0 1
    135.009 55.304 295.274 18 1 1 1742 1 
    90.000 0.000 0.000 19 1 1 0 1 
       ..  

% Auto generated parameter table will be inserted here

## References

[1] R.A. Lebensohn, 2001. N-site modeling of a 3D viscoplastic polycrystal using Fast Fourier Transform. Acta mater. 49, 2723-2737.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
