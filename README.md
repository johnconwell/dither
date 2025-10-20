# Dither

A command line interface application that provides several image processing functions that can be applied to PNG images.

## Usage
Use the `dither` command followed by the input and output flags and any number of optional flags. See the example section for a detailed look at use cases.

## CLI
`dither [options]`
| Flag | Description | Necessity |
| :-- | :-- | :-- |
| `-h, --help` | Displays help | Optional |
| `-i, --input <FILE_PATH>` | Specifies the input png image file path | Required |
| `-o, --output <FILE_PATH>` | Specifies the file path the output png will be saved to | Required |
| `-a, --gamma_correction` | Applies gamma correction the the input image | Optional |
| `-g, --grayscale <GRAYSCALE_WEIGHTS>` | Converts the input image to grayscale using the specified channel weights | Optional |
| `-c, --convolve <KERNEL>` | Convolves the input image using the specified kernel | Optional |
| `-r, --reduce <MAPPING_METHOD>,<PALETTE>` | Reduces the image to the specified palette without applying dithering | Optional |
| `-e, --error_diffusion <ALGORITHM>,<SCAN_PATTERN>,<MAPPING_METHOD>,<PALETTE>` | Dithers the image using the specified error diffusion algorithm and color mapping method | Optional |
| `-d, --ordered <THRESHOLD_MATRIX>,<MAPPING_METHOD>,<PALETTE>` | Dithers the image using the specified threshold matrix and color mapping method | Optional |
| `-t, --temporal <TEMPORAL_METHOD>,<TEMPORAL_FRAMES>,<MAPPING_METHOD>,<PALETTE>` | Dithers the image temporally using the specified temporal dithering method, color mapping method, and number of frames. Output can be saved to .gif or .png | Optional |
| `-b, --benchmark` | Displays benchmark information to stdout | Optional |

### Grayscale
Images can be converted to greyscale with the `-g, --grayscale` command.</br>
`dither -i INPUT_FILE_PATH -o OUTPUT_FILE_PATH -g GRAYSCALE_WEIGHTS`</br>
See [Grayscale Channel Weights](#grayscale-channel-weights) for available RGB channel weighting.

### Convolution
Images can be convolved with the `-c, --convolve` command</br>
`dither -i INPUT_FILE_PATH -o OUTPUT_FILE_PATH -c KERNEL`</br>
The input file and output files must be .png.</br>
See [Convolution Kernels](#convolution-kernels) for available kernels.

### Reduce
Images can have their palette reduced with the `-r, --reduce` command</br>
`dither -i INPUT_FILE_PATH -o OUTPUT_FILE_PATH -r MAPPING_METHOD,PALETTE`</br>
The input file and output files must be .png.</br>
See [Palette Mapping Methods](#palette-mapping-methods) for available color mapping methods from input image pixel colors to palette colors.</br>
See [Palettes](#palettes) for available palettes to map to.

### Error Diffusion Dithering
Images can be dithered using an error diffusion algorithm with the `-e, --error-diffusion` command</br>
`dither -i INPUT_FILE_PATH -o OUTPUT_FILE_PATH -e ALGORITHM,MAPPING_METHOD,PALETTE`</br>
The input file and output files must be .png.</br>
See [Error Diffusion Algorithms](#error-diffusion-algorithms) for available error diffusion algorithms (error kernels).</br>
See [Palette Mapping Methods](#palette-mapping-methods) for available color mapping methods from input image pixel colors to palette colors.</br>
See [Palettes](#palettes) for available palettes to map to.

### Ordered Dithering
Images can be dithered using a threshold matrix with the `-d, --ordered` command</br>
`dither -i INPUT_FILE_PATH -o OUTPUT_FILE_PATH -d THRESHOLD_MATRIX,MAPPING_METHOD,PALETTE`</br>
The input file and output files must be .png.</br>
See [Ordered Threshold Matrixes](#ordered-threshold-matrixes) for available threshold matrixes.</br>
See [Palette Mapping Methods](#palette-mapping-methods) for available color mapping methods from input image pixel colors to palette colors.</br>
See [Palettes](#palettes) for available palettes to map to.

### Temporal Dithering
Images can be dithered in the time dimension with the `-t, --temporal` command</br>
`dither -i INPUT_FILE_PATH -t TEMPORAL_METHOD,TEMPORAL_FRAMES,MAPPING_METHOD,PALETTE`</br>
The input file must be .png. The output file may be .gif or .png.</br>
If the output file is a .gif, then the output will be a gif with `TEMPORAL_FRAMES` frames.</br>
If the output file is a .png, then the output will be a png with `TEMPORAL_FRAMES` frames arranged vertically.</br>
See [Temporal Dithering Methods](#temporal-dithering-methods) for available threshold matrixes</br>
See [Palette Mapping Methods](#palette-mapping-methods) for available color mapping methods from input image pixel colors to palette colors.</br>
See [Palettes](#palettes) for available palettes to map to.

### Grayscale Channel Weights
Options when using the `-g, --grayscale` flag. Each pixel in the input image will be converted to grayscale according to the weights specified by the option.
| GRAYSCALE_WEIGHTS | Description |
| :-- | :-- |
| `STANDARD` | R: 0.3334<br/>G: 0.3333<br/>B: 0.3333 |
| `BT709` | R: 0.2126<br/>G: 0.7152<br/>B: 0.0722 |
| `CHANNEL_R` | R: 1.0<br/>G: 0.0<br/>B: 0.0 |
| `CHANNEL_G` | R: 0.0<br/>G: 1.0<br/>B: 0.0 |
| `CHANNEL_B` | R: 0.0<br/>G: 0.0<br/>B: 1.0 |
| `CHANNEL_RG` | R: 0.5<br/>G: 0.5<br/>B: 0.0 |
| `CHANNEL_RB` | R: 0.5<br/>G: 0.0<br/>B: 0.5 |
| `CHANNEL_GB` | R: 0.0<br/>G: 0.5<br/>B: 0.5 |

### Convolution Kernels
Options when using the `-c, --convolve` flag. The input image will be convolved against the kernel specified by the option.
| KERNEL | Description |
| :-- | :-- |
| `RIDGE_4` | { 0,-1, 0}<br/>{-1, 4,-1}<br/>{ 0,-1, 0} |
| `RIDGE_8` | {-1,-1,-1}<br/>{-1, 8,-1}<br/>{-1,-1,-1} |
| `SHARPEN_4` | { 0,-1, 0}<br/>{-1, 5,-1}<br/>{ 0,-1, 0} |
| `SHARPEN_8` | {-1,-1,-1}<br/>{-1, 9,-1}<br/>{-1,-1,-1} |
| `BOX_BLUR` | { 1, 1, 1}<br/>{ 1, 1, 1} * (1 / 9)<br/>{ 1, 1, 1} |
| `GAUSSIAN_BLUR` | 5x5 Gaussian function matrix, sigma = 1.0 |
| `UNSHARP_MASK` | { 0, 0, 0, 0, 0}<br/>{ 0, 0, 0, 0, 0}<br/>{ 0, 0, 2, 0, 0} - GAUSSIAN_BLUR <br/>{ 0, 0, 0, 0, 0}<br/>{ 0, 0, 0, 0, 0}<br/>

### Error Diffusion Algorithms
Algorithm options when using the `-e, --error_diffusion` flag. The input image will be quantized using the algorithm specified by the option. The 'X' in each kernel represents the pixel being quantized, and the rest of the kernel represents the fraction of error that will be diffused to the surrounding pixels. The bottom fraction in parentheses is a multiplier applied to each non-'X' member of the kernel.
| ALGORITHM | Description |
| :-- | :-- |
| `LINEAR` | {X, 1} |
| `FLOYD_STEINBERG` | {0, X, 7}<br/>{3, 5, 1}<br/>(1 / 16)|
| `JARVICE_JUDICE_NINKE` | {0, 0, X, 7, 5}<br/>{3, 5, 7, 5, 3}<br/>{1, 3, 5, 3, 1}<br/>(1 / 48) |
| `STUCKI` | {0, 0, X, 8, 4}<br/>{2, 4, 8, 4, 2}<br/>{1, 2, 4, 2, 1}<br/>(1 / 42) |
| `ATKINSON` | {0, X, 1, 1}<br/>{1, 1, 1, 0}<br/>{0, 1, 0, 0}<br/>(1 / 8) |
| `BURKES` | {0, 0, X, 8, 4}<br/>{2, 4, 8, 4, 2}<br/>(1 / 32) |
| `SIERRA` | {0, 0, X, 5, 3}<br/>{2, 4, 5, 4, 2}<br/>{0, 2, 3, 2, 0}<br/>(1 / 32) |
| `SIERRA_TWO_ROW` | {0, 0, X, 4, 3}<br/>{1, 2, 3, 2, 1}<br/>(1 / 16) |
| `SIERRA_LITE` | {0, X, 2}<br/>{1, 1, 0}<br/>(1 / 4) |

### Error Diffusion Scan Patterns
Scan Pattern options when using the `-e, --error_diffusion` flag. The scan pattern determines the order in which pixels are processed or quantized in the image.
| SCAN_PATTERN | Description |
| :-- | :-- |
| `RASTER` | Pixels will be quantized from left to right, starting at the top row and working down |
| `SERPENTINE` | Pixels will be quantized from left to right on odd rows and right to left on even rows, starting at the top row and working down |

### Ordered Threshold Matrixes
Threshold matrix (or "map") options when using the `-d, --ordered` flag. The input image will be quantized against the threshold map specified by the option.<br/>
[Bayer](https://en.wikipedia.org/wiki/Ordered_dithering#Threshold_map) pattern threshold matrixes produce even results but leave noticable patterns in the output image. Bayer matrixes larger than 8x8 are unlikely to produce better results, since an 8x8 bayer matrix contains every possible value (0-255) of an sRGB encoded image.<br/>
[Blue Noise](https://github.com/johnconwell/noise2d) threshold matrixes contain only high-frequency noise, producing even results with few noticalbe patterns, especially when using larger threshold matrixes.<br/>
[White Noise](https://en.wikipedia.org/wiki/White_noise) threshold matrixes produce uneven results and leave noticable patterns in the output image. They are included for comparison.
| THRESHOLD_MATRIX | Description |
| :-- | :-- |
| `BAYER_2X2` | 2x2 Bayer pattern threshold matrix |
| `BAYER_4X4` | 4x4 Bayer pattern threshold matrix |
| `BAYER_8X8` | 8x8 Bayer pattern threshold matrix |
| `BAYER_16X16` | 16x16 Bayer pattern threshold matrix |
| `BLUE_NOISE_2X2` | 2x2 blue noise threshold matrix |
| `BLUE_NOISE_4X4` | 4x4 blue noise threshold matrix |
| `BLUE_NOISE_8X8` | 8x8 blue noise threshold matrix |
| `BLUE_NOISE_16X16` | 16x16 blue noise threshold matrix |
| `BLUE_NOISE_32X32` | 32x32 blue noise threshold matrix |
| `BLUE_NOISE_64X64` | 64x64 blue noise threshold matrix |
| `BLUE_NOISE_128X128` | 128x128 blue noise threshold matrix |
| `BLUE_NOISE_256X256` | 256x256 blue noise threshold matrix |
| `WHITE_NOISE_2X2` | 2x2 white noise threshold matrix |
| `WHITE_NOISE_4X4` | 4x4 white noise threshold matrix |
| `WHITE_NOISE_8X8` | 8x8 white noise threshold matrix |
| `WHITE_NOISE_16X16` | 16x16 white noise threshold matrix |
| `WHITE_NOISE_32X32` | 32x32 white noise threshold matrix |
| `WHITE_NOISE_64X64` | 64x64 white noise threshold matrix |
| `WHITE_NOISE_128X128` | 128x128 white noise threshold matrix |
| `WHITE_NOISE_256X256` | 256x256 white noise threshold matrix |

### Temporal Dithering Methods

| TEMPORAL_METHOD | Description |
| :-- | :-- |
| `RANDOM` | Randomly assigns each pixel of each frame to any palette color, weighted based on color distance |
| `PWM` | Modulates each pixel between the nearest lighter and darker palette colors |

### Palette Mapping Methods
Mapping method options when using the `-r, --reduce`, `-e, --error_diffusion`, `-d, --ordered`, or `-t, --temporal` flags. Each pixel in the output image will be mapped to a color in the palette according to the specified option. Pixels are mapped after accumulated error is applied in error diffusion algorithms and after the threshold matrix is applied in ordered algorithms.
| MAPPING_METHOD | Description |
| :-- | :-- |
| `EUCLIDEAN_DISTANCE` | Maps each pixel to the nearest palette color in Euclidean space |
| `MANHATTAN_DISTANCE` | Maps each pixel to the nearest palette color in Cartesian space |
| `UNIFORM_HISTOGRAM` | Maps each pixel such that the output image contains approximately equal amount of each color in the specified palette. Note that the resulting image may not have a uniform distribution of palette colors, but will likely have a more uniform distribution than if choosing a distance-based method. Not valid when using `-t` flag. |

### Palettes
Palette options when using the `-r, --reduce`, `-e, --error_diffusion`, `-d, --ordered`, or `-t, --temporal` flags. The following palettes are built in and can be referenced by name.
| PALETTE | Description |
| :-- | :-- |
| 1BIT_MONITOR_GLOW | [lospec](https://lospec.com/palette-list/1bit-monitor-glow) |
| NOIRE_TRUTH | [lospec](https://lospec.com/palette-list/noire-truth) |
| TITANSTONE | [lospec](https://lospec.com/palette-list/titanstone) |
| ICE_CREAM_GB | [lospec](https://lospec.com/palette-list/ice-cream-gb) |
| MIST_GB | [lospec](https://lospec.com/palette-list/mist-gb) |
| CRIMSON | [lospec](https://lospec.com/palette-list/crimson) |
| FIERY_PLAGUE_GB | [lospec](https://lospec.com/palette-list/fiery-plague-gb) |
| TWILIGHT5 | [lospec](https://lospec.com/palette-list/twilight-5) |
| OIL6 | [lospec](https://lospec.com/palette-list/oil-6) |
| MIDNIGHT_ABLAZE | [lospec](https://lospec.com/palette-list/midnight-ablaze) |
| SLSO8 | [lospec](https://lospec.com/palette-list/slso8) |

Alternatively, specify a file path containing a .csv file in the format</br>
`NAME,AUTHOR,COLOR1,COLOR2,...`</br>
where `COLORX` is a hexadecimal color code with no additional prefixes. This format exactly matches the Lospec API, so any palette csv can be obtained by appending ".csv" to the URL of a lospec palette. See examples in the [palettes](./data/palettes/) folder.

## Examples
The below examples are executed from the root directory. `dither` can be used instead of `.\bin\dither.exe` if the executable is added to your path. Input and output directories are shown relative to the root directory, but absolute paths can be used as well.</br>
`.\bin\dither.exe -i .\input\baboon.png -o .\output\baboon_grayscale_bt709.png -g BT709`</br>
`.\bin\dither.exe -i .\input\boat.png -o .\output\boat_reduce_twilight5.png -r MANHATTAN_DISTANCE,TWILIGHT5`</br>
`.\bin\dither.exe -i .\input\lena.png -o .\output\lena_convolve_unsharp_mask.png -c UNSHARP_MASK`</br>
`.\bin\dither.exe -i .\input\earth.png -o .\output\earth_error_diffusion_jarvice-judice-ninke_raster_twilight5.png -e JARVICE_JUDICE_NINKE,RASTER,EUCLIDEAN_DISTANCE,TWILIGHT5`</br>
`.\bin\dither.exe -i .\input\lena.png -o .\output\lena_error_diffusion_atkinson_serpentine_midnight_ablaze.png -e ATKINSON,SERPENTINE,EUCLIDEAN_DISTANCE,MIDNIGHT_ABLAZE -a`</br>
`.\bin\dither.exe -i .\input\peppers.png -o .\output\peppers_ordered_blue_noise_64x64_cherrymelon.png -d BLUE_NOISE_64X64,EUCLIDEAN_DISTANCE,.\data\palettes\cherrymelon.csv`</br>
`.\bin\dither.exe -i .\input\baboon.png -o .\output\baboon_ordered_bayer_16x16_witching-hour.png -d BAYER_16X16,EUCLIDEAN_DISTANCE,.\data\palettes\witching-hour.csv -a`</br>
`.\bin\dither.exe -i .\input\tulips.png -o .\output\tulips_temporal_pwm_slso8.gif -t PWM,8,EUCLIDEAN_DISTANCE,SLSO8 -a`</br>
`.\bin\dither.exe -i .\input\tulips.png -o .\output\tulips_temporal_pwm_noire_truth.png -t PWM,4,EUCLIDEAN_DISTANCE,NOIRE_TRUTH -a`</br>

## To Compile
Requirements: GNU C++ compiler, make<br/>
Open a terminal in the root directory and enter the command
```bash
> make
```

## Why Use Dither Instead of ImageMagick?


## Libraries
[Noise2D](https://github.com/johnconwell/noise2d) - used to generate threshold matrixes for ordered dithering</br>
[LodePNG](https://lodev.org/lodepng/) - provides png encoding/decoding functions</br>
[gif-h](https://github.com/charlietangora/gif-h) - provides gif writing functions
