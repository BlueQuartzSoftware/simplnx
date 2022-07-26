# ITKRefineTileCoordinates.h  #

## Group (Subgroup) ##

ImageProcessing (ImageProcessing)


## Description ##

This filter takes a series of tiled gray-scale images (8bit) and calculates the origin of each tile such that a fully stitched montage would result from placing each tile. The images, and only the images, must all be sitting in one attribute matrix.

When it comes to montage assembly the order that the images are compared is important. For that reason the imported images must be in a certain order when this filter is run. The filter currently supports Row-By-Row, Column-By-Column,and Snake-By-Row. Snake-By-Column is also included but can be buggy, it is recommended that you choose a different option.

The images are then manually re-ordered as though they were captured in a row-by-row comb of the data ![](Images/RowWiseComb.png)

This re-ordering is still only done locally for the filter, any output data will correspond to how the images were originally ordered.


The first image is given coordinates of (0,0). The second is by placed by taking the overlap window between the first image and the second image and using cross correlation to find the location of maximum overlap of the image data. ![](Images/LeftXC.png) The overlap window is found from the global stage positions of the tiles in the meta data. All images in the first row are placed by comparing a shared window on the left of the image with a shared window on the right of the previously placed image. 

For images in the first column, the overlap window is found by taking the top of the image to be placed with the bottom of the image that is already placed. ![](Images/TopXC.png)

For all other images, both a top and left window are taken, and the best position is averaged. ![](Images/TopAndLeftXC.png)

When running the cross-correlation, a requirement of at least 50% overlap of the two windows is placed on the operation. 

This filter uses the *FFTNormalizedCorrelationImageFilter* from the ITK library. 

The result of this filter is an array containing the global xy origins of each tile (with (0, 0) being the origin of the first tile). In order to actually stitch the images and put into a new data array, the *Stitch Images* filter must be called after this one. 


There is an option for the use of Zeiss meta data with this filter. This is a legacy option in that it is outdated and unsupported. If you use the Zeiss data there is a high chance that the filter will not work.


## Parameters ##

Import Mode - The order of the images within the attribute matrix

Tile Dimensions - How many images there are per row and column of the montage

Overlap Percentage - The estimated overlap of the images ontop of each other.

Cell Attribute Matrix - The attribute matrix that holds the images.


An attribute matrix that holds all the images (and only the images) of uint8 type is required.


## Created Arrays ##

An attribute matrix to hold the above arrays is also created. The default name is "Tile Info AttrMat". 
 This holds the coordinate data, as well as the names of the corresponding images.


## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


