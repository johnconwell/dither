# Dither — Market Comparison

*Prepared October 30, 2026. Feature claims for other tools come from search results and package listings; they were not tested hands-on and may have changed.*

## Summary
Dither's closest competitor is **didder** (Go, open source, command line). Competitors are generally ahead on **getting images in** (formats, resizing), **adjusting them first** (strength, tone controls) and **building palettes** (automatic quantization). Dither's clearest differentiator is **temporal dithering**, which no other tool reviewed offers.

## The market at a glance

| Segment | Examples | What they compete on |
| :-- | :-- | :-- |
| Command-line / scripting | ImageMagick, didder, pngquant, Aseprite's command line | Pipelines, file formats, batch jobs |
| Free GUI / web | Dithermark, Ditherista, Dithertron, PIXATTO | Live preview, number of algorithms, ease of use |
| Paid creative apps | Dither Boy (£45), DitherMaster ($8+), Dithertime ($7.50+) | Effects, video, presets, community |
| Video plugins | DitherFactory (€39), Red Giant Pixel Dither, RetroDither | After Effects workflows |

Reference points:
- **Dither Boy 6** has 63 algorithms and stackable effects, and exports PNG, SVG, JPEG and MP4.
- **didder's author also sells a GUI**, Dithertime, which suggests there is demand for a graphical companion to a command-line tool.

## What other tools have that Dither lacks

### Input and pre-processing
- **Input formats.** Dither reads only PNG. didder reads JPEG, GIF (static), PNG, BMP and TIFF, and applies EXIF rotation.
- **Resizing.** didder can downscale before dithering and upscale by integer factors afterwards without artifacts. Most web tools have a "pixel size" control.
- **Tone controls.** didder has brightness, contrast and saturation; Ditherista has brightness, contrast and gamma.

### Dithering controls
- **Strength / amount.** didder has `--strength` (about 64% is recommended for color Bayer dithering); pngquant has a dithering level from 0 to 1. Dither always dithers at 100%. This is probably the biggest quality gap for ordered dithering in color.
- **Custom error-diffusion kernels.** Dither accepts CSV files for threshold matrices, convolution kernels and grayscale weights, but not for error-diffusion kernels. didder takes them as JSON. This is inconsistent with Dither's CSV-for-everything approach and is likely an easy addition.
- **Recolor after dithering.** didder can dither against a grayscale palette, then swap in a single-hue palette (e.g. Game Boy greens), which gives better results with low-contrast palettes.

### Palettes and color
- **Automatic palette generation.** Dither needs a fixed palette. pngquant generates optimal palettes and picks the color count automatically; Ditherista offers three quantization methods; ImageMagick has `-colors` and k-means. Without this, Dither can't handle "reduce this photo to 16 good colors."
- **Perceptual color matching.** Dither matches colors with weighted RGB Euclidean or Manhattan distance. Perceptual spaces (CIELAB, OKLab) usually match better with unusual palettes. Ditherista advertises many color-matching methods.

### Algorithm range
- **libdither / Ditherista** (90+ methods): variable error diffusion (Ostromoukhov, Zhou Fang), pattern dithering, Direct Binary Search, dot diffusion, Kacker–Allebach.
- **ImageMagick:** halftone and circle threshold maps, Riemersma dithering.
- **didder:** clustered-dot matrices, horizontal and vertical line matrices.
- Dither has **no halftone or clustered-dot option**, which print-style and risograph-style users want.

### Workflow and distribution
- **Batch and pipes.** didder handles many files per command, can combine frames into an animated GIF and uses every CPU core. pngquant supports stdin/stdout. Dither handles one file per run.
- **Packaging.** didder is in Debian and on Homebrew. Dither must be compiled with GCC 13+, and its README examples assume Windows paths.
- **Retro-hardware rules.** Dithertron enforces per-block color limits for the C64, ZX Spectrum, NES and others.
- **GUI, video, vector output.** All GUI tools have live preview. Dither Boy and the After Effects plugins handle video, and Dither Boy exports SVG. Possibly out of scope, but it's where most of the paid market is.
- **Library / API.** libdither and libimagequant can be embedded in other programs; Dither is a standalone executable only.

## What Dither has that's hard to find elsewhere

1. **Temporal dithering (PWM, stochastic PWM, random).** Spreads each pixel across N frames of a GIF or a vertical sprite sheet. The PWM method is deterministic, and each pixel's starting frame comes from a blue-noise matrix so every frame has about the same brightness. None of the tools reviewed offer this. Dither Boy 6's "temporal variation" sounds similar but is an animated-noise effect, not color reproduction over time. The sprite-sheet output is especially useful for game developers. **This should be the headline feature.**
2. **Histogram equalisation onto the palette (`UNIFORM_HISTOGRAM`).** Gives roughly equal coverage of each palette color. Not found in any competitor reviewed.
3. **Large built-in blue-noise matrices (up to 256×256) and custom CSV matrices.** ImageMagick has no blue noise, and didder still lists it as in progress. Some GUI tools have blue noise, so this is an advantage rather than unique.
4. **Grayscale, convolution and dithering in one command, all optionally in linear light,** respecting the PNG `gAMA` chunk. This matches the most careful tools (didder, Ditherista, pngquant) and beats most web tools.
5. **Lospec CSV palettes load directly.** Convenient, though PIXATTO and DitherMaster accept Lospec `.hex` files.
6. **Engineering quality:** deterministic output, clear errors, a test suite with `XFAIL` tracking, a `--benchmark` flag and few dependencies. This matters to people building pipelines.

### Note on the README's "Why use Dither instead of ImageMagick?" section
Two points overstate the case. ImageMagick has built-in convolution kernels (`-morphology Convolve`) and supports arbitrary channel weights (`-color-matrix`, `-fx`). The temporal dithering, blue noise, histogram mapping and error-diffusion-kernel points are fair. Lead with those and soften or drop the other two.

## Suggested priorities
1. **Quick wins:** a dithering strength setting, custom error-diffusion kernels as CSV, JPEG/BMP input, `--resize` / `--upscale`, then batch and stdin/stdout support.
2. **Biggest gap:** automatic palette generation (median-cut or k-means).
3. **Adoption:** packaged releases (Homebrew, a .deb, prebuilt Windows binaries) would likely matter more than any new algorithm.
4. **Positioning:** feature temporal dithering and histogram mapping at the top of the README.

## Sources
- [didder on GitHub](https://github.com/makew0rld/didder)
- [didder Debian package](https://software.pureos.net/package/bin/pureos/dawn/didder)
- [didder man page](https://dyn.manpages.debian.org/unstable/didder/didder.1.en.html)
- [Dither Boy 6.0, Digital Production](https://digitalproduction.com/2026/03/18/dither-boy-6-0-brings-animation-and-video-polish/)
- [DitherFactory, Digital Production](https://digitalproduction.com/2026/10/27/ditherfactory-after-effects-dithering/)
- [Dithertime on itch.io](https://makew0rld.itch.io/dithertime)
- [DitherMaster on itch.io](https://bigjk.itch.io/dithermaster)
- [libdither](https://github.com/ImageProcessing-ElectronicPublications/libdither)
- [Ditherista on GitHub](https://github.com/robertkist/ditherista)
- [Ditherista on AlternativeTo](https://alternativeto.net/software/ditherista/about)
- [PIXATTO on itch.io](https://vorvek.itch.io/pixatto)
- [ImageMagick thresholds.xml](https://legacy.imagemagick.org/source/thresholds.xml)
- [pngquant](https://github.com/IOS-Project-Development/pngquant)
- [Dithertron / 8bitworkshop 3.6.0](https://8bitworkshop.com/docs/posts/2020/8bitworkshop-ide-release-3-6-0.html)
- [Aseprite command line](https://www.aseprite.org/cli/)
