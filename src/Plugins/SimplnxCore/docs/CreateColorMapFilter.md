# Create Color Map

## Description

This **Filter** generates a color table array for a given 1-component input array.  Each element of the input array
is normalized and converted to a color based on where the value falls in the spectrum of the selected color preset.

The user can apply an optional data mask and then set the RGB values (0-255) that will be used if the data mask has a FALSE
value.

## Preset Values

These are the valid preset strings that can be used.

| Preset Name | Color Space | Example |
|-------------|-------------|---------|
| 2hot | Lab | ![2hot](Images/ColorTable_2hot.png) |
| Asymmetrical Earth Tones (6_21b) | Lab | ![Asymmetrical Earth Tones (6_21b)](Images/ColorTable_Asymmetrical_Earth_Tones_(6_21b).png) |
| Black-Body Radiation | RGB | ![Black-Body Radiation](Images/ColorTable_Black-Body_Radiation.png) |
| Black, Blue and White | RGB | ![Black, Blue and White](Images/ColorTable_Black_Blue_and_White.png) |
| Black, Orange and White | RGB | ![Black, Orange and White](Images/ColorTable_Black_Orange_and_White.png) |
| Blue - Green - Orange | CIELAB | ![Blue - Green - Orange](Images/ColorTable_Blue_Green_Orange.png) |
| Blue Orange (divergent) | Lab | ![Blue Orange (divergent)](Images/ColorTable_Blue_Orange_(divergent).png) |
| Blue to Yellow | RGB | ![Blue to Yellow](Images/ColorTable_Blue_to_Yellow.png) |
| BLUE-WHITE | Lab | ![BLUE-WHITE](Images/ColorTable_BLUE-WHITE.png) |
| blue2cyan | Lab | ![blue2cyan](Images/ColorTable_blue2cyan.png) |
| blue2yellow | Lab | ![blue2yellow](Images/ColorTable_blue2yellow.png) |
| Blues | Lab | ![Blues](Images/ColorTable_Blues.png) |
| bone_Matlab | Lab | ![bone_Matlab](Images/ColorTable_bone_Matlab.png) |
| BrBG | Lab | ![BrBG](Images/ColorTable_BrBG.png) |
| BrOrYl | Lab | ![BrOrYl](Images/ColorTable_BrOrYl.png) |
| BuGn | Lab | ![BuGn](Images/ColorTable_BuGn.png) |
| BuGnYl | Lab | ![BuGnYl](Images/ColorTable_BuGnYl.png) |
| BuPu | Lab | ![BuPu](Images/ColorTable_BuPu.png) |
| BuRd | Lab | ![BuRd](Images/ColorTable_BuRd.png) |
| CIELab Blue to Red | Lab | ![CIELab Blue to Red](Images/ColorTable_CIELab_Blue_to_Red.png) |
| Cividis | Lab | ![Cividis](Images/ColorTable_Cividis.png) |
| Cold and Hot | RGB | ![Cold and Hot](Images/ColorTable_Cold_and_Hot.png) |
| Cool to Warm (Extended) | Lab | ![Cool to Warm (Extended)](Images/ColorTable_Cool_to_Warm_(Extended).png) |
| Cool to Warm | Diverging | ![Cool to Warm](Images/ColorTable_Cool_to_Warm.png) |
| copper_Matlab | Lab | ![copper_Matlab](Images/ColorTable_copper_Matlab.png) |
| erdc_blue_BW | Lab | ![erdc_blue_BW](Images/ColorTable_erdc_blue_BW.png) |
| erdc_blue2cyan_BW | Lab | ![erdc_blue2cyan_BW](Images/ColorTable_erdc_blue2cyan_BW.png) |
| erdc_blue2gold | Lab | ![erdc_blue2gold](Images/ColorTable_erdc_blue2gold.png) |
| erdc_blue2gold_BW | Lab | ![erdc_blue2gold_BW](Images/ColorTable_erdc_blue2gold_BW.png) |
| erdc_blue2green_BW | Lab | ![erdc_blue2green_BW](Images/ColorTable_erdc_blue2green_BW.png) |
| erdc_blue2green_muted | Lab | ![erdc_blue2green_muted](Images/ColorTable_erdc_blue2green_muted.png) |
| erdc_blue2yellow | Lab | ![erdc_blue2yellow](Images/ColorTable_erdc_blue2yellow.png) |
| erdc_brown_BW | Lab | ![erdc_brown_BW](Images/ColorTable_erdc_brown_BW.png) |
| erdc_cyan2orange | Lab | ![erdc_cyan2orange](Images/ColorTable_erdc_cyan2orange.png) |
| erdc_divHi_purpleGreen | Lab | ![erdc_divHi_purpleGreen](Images/ColorTable_erdc_divHi_purpleGreen.png) |
| erdc_divHi_purpleGreen_dim | Lab | ![erdc_divHi_purpleGreen_dim](Images/ColorTable_erdc_divHi_purpleGreen_dim.png) |
| erdc_divLow_icePeach | Lab | ![erdc_divLow_icePeach](Images/ColorTable_erdc_divLow_icePeach.png) |
| erdc_divLow_purpleGreen | Lab | ![erdc_divLow_purpleGreen](Images/ColorTable_erdc_divLow_purpleGreen.png) |
| erdc_gold_BW | Lab | ![erdc_gold_BW](Images/ColorTable_erdc_gold_BW.png) |
| erdc_green2yellow_BW | Lab | ![erdc_green2yellow_BW](Images/ColorTable_erdc_green2yellow_BW.png) |
| erdc_iceFire_H | Lab | ![erdc_iceFire_H](Images/ColorTable_erdc_iceFire_H.png) |
| erdc_iceFire_L | Lab | ![erdc_iceFire_L](Images/ColorTable_erdc_iceFire_L.png) |
| erdc_magenta_BW | Lab | ![erdc_magenta_BW](Images/ColorTable_erdc_magenta_BW.png) |
| erdc_marine2gold_BW | Lab | ![erdc_marine2gold_BW](Images/ColorTable_erdc_marine2gold_BW.png) |
| erdc_orange_BW | Lab | ![erdc_orange_BW](Images/ColorTable_erdc_orange_BW.png) |
| erdc_pbj_lin | Lab | ![erdc_pbj_lin](Images/ColorTable_erdc_pbj_lin.png) |
| erdc_purple_BW | Lab | ![erdc_purple_BW](Images/ColorTable_erdc_purple_BW.png) |
| erdc_purple2green | Lab | ![erdc_purple2green](Images/ColorTable_erdc_purple2green.png) |
| erdc_purple2green_dark | Lab | ![erdc_purple2green_dark](Images/ColorTable_erdc_purple2green_dark.png) |
| erdc_purple2pink_BW | Lab | ![erdc_purple2pink_BW](Images/ColorTable_erdc_purple2pink_BW.png) |
| erdc_rainbow_bright | Lab | ![erdc_rainbow_bright](Images/ColorTable_erdc_rainbow_bright.png) |
| erdc_rainbow_dark | Lab | ![erdc_rainbow_dark](Images/ColorTable_erdc_rainbow_dark.png) |
| erdc_red_BW | Lab | ![erdc_red_BW](Images/ColorTable_erdc_red_BW.png) |
| erdc_red2purple_BW | Lab | ![erdc_red2purple_BW](Images/ColorTable_erdc_red2purple_BW.png) |
| erdc_red2yellow_BW | Lab | ![erdc_red2yellow_BW](Images/ColorTable_erdc_red2yellow_BW.png) |
| erdc_sapphire2gold_BW | Lab | ![erdc_sapphire2gold_BW](Images/ColorTable_erdc_sapphire2gold_BW.png) |
| Fast | RGB | ![Fast](Images/ColorTable_Fast.png) |
| GBBr | Lab | ![GBBr](Images/ColorTable_GBBr.png) |
| gist_earth | Lab | ![gist_earth](Images/ColorTable_gist_earth.png) |
| GnBu | Lab | ![GnBu](Images/ColorTable_GnBu.png) |
| GnBuPu | Lab | ![GnBuPu](Images/ColorTable_GnBuPu.png) |
| GnRP | Lab | ![GnRP](Images/ColorTable_GnRP.png) |
| GnYlRd | Lab | ![GnYlRd](Images/ColorTable_GnYlRd.png) |
| Gray and Red | Lab | ![Gray and Red](Images/ColorTable_Gray_and_Red.png) |
| Grayscale | RGB | ![Grayscale](Images/ColorTable_Grayscale.png) |
| Green-Blue Asymmetric Divergent (62Blbc) | Lab | ![Green-Blue Asymmetric Divergent (62Blbc)](Images/ColorTable_Green-Blue_Asymmetric_Divergent_(62Blbc).png) |
| GREEN-WHITE_LINEAR | Lab | ![GREEN-WHITE_LINEAR](Images/ColorTable_GREEN-WHITE_LINEAR.png) |
| Greens | Lab | ![Greens](Images/ColorTable_Greens.png) |
| GYPi | Lab | ![GYPi](Images/ColorTable_GYPi.png) |
| GyRd | Lab | ![GyRd](Images/ColorTable_GyRd.png) |
| Haze | RGB | ![Haze](Images/ColorTable_Haze.png) |
| Haze_cyan | Lab | ![Haze_cyan](Images/ColorTable_Haze_cyan.png) |
| Haze_green | Lab | ![Haze_green](Images/ColorTable_Haze_green.png) |
| Haze_lime | Lab | ![Haze_lime](Images/ColorTable_Haze_lime.png) |
| heated_object | Lab | ![heated_object](Images/ColorTable_heated_object.png) |
| hsv | RGB | ![hsv](Images/ColorTable_hsv.png) |
| hue_L60 | Lab | ![hue_L60](Images/ColorTable_hue_L60.png) |
| Inferno (matplotlib) | Diverging | ![Inferno (matplotlib)](Images/ColorTable_Inferno_(matplotlib).png) |
| Jet | RGB | ![Jet](Images/ColorTable_Jet.png) |
| Linear Blue (8_31f) | Lab | ![Linear Blue (8_31f)](Images/ColorTable_Linear_Blue_(8_31f).png) |
| Linear Green (Gr4L) | Lab | ![Linear Green (Gr4L)](Images/ColorTable_Linear_Green_(Gr4L).png) |
| Linear YGB 1211g | Lab | ![Linear YGB 1211g](Images/ColorTable_Linear_YGB_1211g.png) |
| magenta | Lab | ![magenta](Images/ColorTable_magenta.png) |
| Magma (matplotlib) | Diverging | ![Magma (matplotlib)](Images/ColorTable_Magma_(matplotlib).png) |
| Muted Blue-Green | Lab | ![Muted Blue-Green](Images/ColorTable_Muted_Blue-Green.png) |
| nic_CubicL | Lab | ![nic_CubicL](Images/ColorTable_nic_CubicL.png) |
| nic_CubicYF | Lab | ![nic_CubicYF](Images/ColorTable_nic_CubicYF.png) |
| nic_Edge | Lab | ![nic_Edge](Images/ColorTable_nic_Edge.png) |
| Oranges | Lab | ![Oranges](Images/ColorTable_Oranges.png) |
| OrPu | Lab | ![OrPu](Images/ColorTable_OrPu.png) |
| pink_Matlab | Lab | ![pink_Matlab](Images/ColorTable_pink_Matlab.png) |
| PiYG | Lab | ![PiYG](Images/ColorTable_PiYG.png) |
| Plasma (matplotlib) | Diverging | ![Plasma (matplotlib)](Images/ColorTable_Plasma_(matplotlib).png) |
| PRGn | Lab | ![PRGn](Images/ColorTable_PRGn.png) |
| PuBu | Lab | ![PuBu](Images/ColorTable_PuBu.png) |
| PuOr | Lab | ![PuOr](Images/ColorTable_PuOr.png) |
| PuRd | Lab | ![PuRd](Images/ColorTable_PuRd.png) |
| Purples | Lab | ![Purples](Images/ColorTable_Purples.png) |
| Rainbow | RGB | ![Rainbow](Images/ColorTable_Rainbow.png) |
| Rainbow Blended Black | RGB | ![Rainbow Blended Black](Images/ColorTable_Rainbow_Blended_Black.png) |
| Rainbow Blended Grey | RGB | ![Rainbow Blended Grey](Images/ColorTable_Rainbow_Blended_Grey.png) |
| Rainbow Blended White | RGB | ![Rainbow Blended White](Images/ColorTable_Rainbow_Blended_White.png) |
| Rainbow Desaturated | RGB | ![Rainbow Desaturated](Images/ColorTable_Rainbow_Desaturated.png) |
| Rainbow Uniform | RGB | ![Rainbow Uniform](Images/ColorTable_Rainbow_Uniform.png) |
| RdOr | Lab | ![RdOr](Images/ColorTable_RdOr.png) |
| RdOrYl | Lab | ![RdOrYl](Images/ColorTable_RdOrYl.png) |
| RdPu | Lab | ![RdPu](Images/ColorTable_RdPu.png) |
| RED_TEMPERATURE | Lab | ![RED_TEMPERATURE](Images/ColorTable_RED_TEMPERATURE.png) |
| RED-PURPLE | Lab | ![RED-PURPLE](Images/ColorTable_RED-PURPLE.png) |
| Reds | Lab | ![Reds](Images/ColorTable_Reds.png) |
| Spectral_lowBlue | Lab | ![Spectral_lowBlue](Images/ColorTable_Spectral_lowBlue.png) |
| Turbo | RGB | ![Turbo](Images/ColorTable_Turbo.png) |
| Viridis (matplotlib) | Diverging | ![Viridis (matplotlib)](Images/ColorTable_Viridis_(matplotlib).png) |
| Warm to Cool (Extended) | Lab | ![Warm to Cool (Extended)](Images/ColorTable_Warm_to_Cool_(Extended).png) |
| Warm to Cool | Diverging | ![Warm to Cool](Images/ColorTable_Warm_to_Cool.png) |
| X Ray | RGB | ![X Ray](Images/ColorTable_X_Ray.png) |
| Yellow - Gray - Blue | Lab | ![Yellow - Gray - Blue](Images/ColorTable_Yellow_Gray_Blue.png) |
| Yellow 15 | Lab | ![Yellow 15](Images/ColorTable_Yellow_15.png) |

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
