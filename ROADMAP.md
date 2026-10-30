# Dither — Feature Roadmap

*Prepared October 30, 2026, from `MARKET_COMPARISON.md` and a read-through of the current source (2.0.0). Effort estimates are based on how the code is structured today.*

## How to read this

Phases are ordered by **impact ÷ effort**. Low-effort items that remove a reason not to use Dither, or that make its unique features easier to find, come first. Higher-effort items that open new use cases come later.

- **Impact:** High = removes a common blocker or adds a headline capability; Medium = a quality or convenience gain for existing users; Low = niche.
- **Effort:** S = small, contained change (about a day); M = new module or cross-cutting change (several days); L = major subsystem (weeks).

## At a glance

| # | Feature | Impact | Effort | Phase |
| :-- | :-- | :-- | :-- | :-- |
| 1 | README repositioning and cross-platform examples | High | S | 0 |
| 2 | Prebuilt releases (Windows, macOS, Linux) via CI | High | S–M | 0 |
| 3 | Package managers (Homebrew tap, Scoop, .deb) | High | S | 0 |
| 4 | Dithering strength (`--strength`) | High | S | 1 |
| 5 | JPEG / BMP / TGA input | High | S | 1 |
| 6 | Custom error-diffusion kernels from CSV | Medium | S | 1 |
| 7 | Clustered-dot, halftone and line matrices | Medium | S | 1 |
| 8 | `--upscale` (integer, nearest neighbour) | Medium | S | 1 |
| 9 | `--recolor` (swap palette after dithering) | Medium | S | 1 |
| 10 | GIF frame delay (`--delay`) | Medium | S | 1 |
| 11 | `--resize` (downscale before dithering) | High | S–M | 2 |
| 12 | stdin / stdout (`-` as a path) | Medium | M | 2 |
| 13 | Batch processing with multithreading | High | M | 2 |
| 14 | Perceptual colour matching (OKLab, CIELAB) | Medium | M | 2 |
| 15 | Tone controls (brightness, contrast, gamma, saturation) | Medium | S–M | 2 |
| 16 | Automatic palette generation | High | M | 3 |
| 17 | Riemersma and variable error diffusion | Medium | M | 4 |
| 18 | Dot diffusion and Direct Binary Search | Low | M–L | 4 |
| 19 | Library API (`libdither` core and header) | Medium | M | 5 |
| 20 | WebAssembly build and browser demo | High | M–L | 5 |
| 21 | Retro-hardware constraints (C64, ZX Spectrum, NES) | Medium | L | 5 |
| 22 | SVG output | Low | M | 5 |

---

## Phase 0: Positioning and distribution (do first)

These involve almost no algorithm work, but they decide whether anyone tries Dither at all. The comparison found that packaging probably matters more than any new algorithm.

**Status (October 30, 2026):** #1–#3 implemented, waiting on the first tagged release. Added: README highlights, install and quick-start sections; `.github/workflows/build.yml` (5 platforms: Linux x86-64/arm64, macOS arm64/Intel, Windows x86-64), `scripts/package.sh`, `scripts/check_deps.sh`, `scripts/render_packaging.sh`, and Homebrew/Scoop templates in `packaging/`. Still to do: create the `homebrew-tap` and `scoop-bucket` repositories, add the `PACKAGING_TOKEN` secret, and push the first `v*` tag. The `.deb` is deferred.

### 1. README repositioning — High / S
- Lead with **temporal dithering** (GIF and sprite-sheet output) and **histogram mapping onto the palette**, with one before/after image for each.
- Rewrite "Why use Dither instead of ImageMagick?": drop or soften the convolution and channel-weights points (ImageMagick has `-morphology Convolve` and `-color-matrix`). Keep temporal, blue noise, histogram mapping and error-diffusion kernels.
- Make examples platform-neutral. Use forward slashes and `dither` instead of Windows-style paths, and add a "Quick start" with three copy-paste commands.

### 2. Prebuilt releases via CI — High / S–M
- A GitHub Actions matrix (Windows with MinGW, macOS, Ubuntu) that runs `make test` and uploads binaries on tags. The Makefile already handles the `.exe` suffix, and the only dependencies are vendored (lodepng, gif-h, cxxopts), so this is mostly CI configuration.
- This removes the "compile with GCC 13+ yourself" barrier, which is the biggest single blocker to adoption.

### 3. Package managers — High / S (after #2)
- A Homebrew tap and a Scoop manifest that point at the release binaries. A `.deb` can come later. didder is already in Debian and Homebrew.

---

## Phase 1: Quick wins

Each of these fits into the existing design with little new structure.

### 4. Dithering strength — High / S
- `--strength <0–1>` (or a percentage). For **ordered** dithering, scale the threshold offset around the midpoint. For **error diffusion**, multiply the diffused error before it is passed to the taps. For **temporal** dithering, scale the duty-cycle offset from the nearest colour.
- This is the biggest quality gap for colour ordered dithering. didder recommends about 64% for colour Bayer.

### 5. JPEG / BMP / TGA input — High / S
- `Image::load` currently calls lodepng directly. Branch on the file's magic bytes: keep lodepng for PNG, so `gAMA`/`sRGB` handling stays intact, and add the single-header `stb_image` for everything else, treated as sRGB (gamma = 0).
- Follow-up (S): read the EXIF orientation tag in JPEGs and rotate, as didder does.

### 6. Custom error-diffusion kernels from CSV — Medium / S
- `ErrorDiffusion`'s constructor already looks up a preset name. Fall back to `csv::read_matrix`, as `Ordered` does, when no preset matches. Use one cell, such as `*` or `0` in the first row, to mark the current pixel, and normalise by the sum of the weights unless the file supplies a divisor.
- This makes "CSV for everything" consistent across threshold matrices, convolution kernels, grayscale weights and diffusion kernels.

### 7. Clustered-dot, halftone and line matrices — Medium / S
- These are ordinary threshold matrices, so they slot into `Ordered`'s presets next to `BAYER_*` and the blue-noise tables. Add, for example, `CLUSTERED_DOT_4X4`, `CLUSTERED_DOT_8X8`, `HALFTONE_8X8` (45° dot), `HORIZONTAL_LINES_4X4` and `VERTICAL_LINES_4X4`.
- This fills the "no halftone option" gap for print-style and risograph-style users at very little cost. Rotated per-channel screens (true CMYK halftone) can come later.

### 8. `--upscale <N>` — Medium / S
- Integer nearest-neighbour scaling applied to every frame just before `save`. This lets pixel-art and sprite-sheet output be shared without blurring.

### 9. `--recolor <PALETTE>` — Medium / S
- After quantising, replace each palette index *i* with colour *i* of a second palette of the same size. Combined with `Palette::sort` (by lightness), this gives the didder-style "dither in grays, then show in Game Boy greens" workflow.

### 10. `--delay <centiseconds>` — Medium / S
- `Image::save` hard-codes a GIF delay of 2. Temporal dithering is the headline feature, so users need control over playback rate (and should be warned that most viewers treat values below 2 as 10).

---

## Phase 2: Pipelines and quality

### 11. `--resize` — High / S–M
- Downscale before grayscale or convolution using an area-average (box) filter in linear light when `-a` is set. Accept `WxH`, `W` or a percentage. Most real inputs are photos far larger than the target, and every web tool has a "pixel size" control.

### 12. stdin / stdout — Medium / M
- Accept `-` for `-i` and `-o`, with `--format png|gif` when the format can't come from an extension. lodepng already decodes from and encodes to memory buffers. gif-h writes through `FILE*`, so it needs a small adapter or a temporary file.

### 13. Batch processing — High / M
- Allow several `-i` values or a glob, with `-o` as a directory or a pattern such as `out/{name}.png`. Process files in parallel with `std::thread` (one image per thread), since each run is independent. Report errors per file and return a non-zero exit code if any file failed.
- This makes Dither a real asset-pipeline tool for game developers, which is the main audience for the sprite-sheet output.

### 14. Perceptual colour matching — Medium / M
- Add `OKLAB_DISTANCE` (and optionally `CIELAB_DE2000`) to `MappingMethod`. `Palette::set_working_space` already converts palette entries once, so store an OKLab copy and convert each input pixel before `nearest_index`. Error diffusion still accumulates error in RGB or linear RGB; only the matching step changes.
- This gives better matches for unusual palettes (Lospec palettes especially).

### 15. Tone controls — Medium / S–M
- `--brightness`, `--contrast`, `--gamma` and `--saturation` as a pre-processing step alongside `-g` and `-c` (applied in that fixed order in `main.cpp`). These are pure per-pixel operations.

---

## Phase 3: Close the biggest gap

### 16. Automatic palette generation — High / M
- Let `<PALETTE>` take a value such as `AUTO_16` (or a separate `--colors 16`) wherever a palette is accepted. Implement **median cut** first (fast and deterministic), then optional **k-means refinement** in OKLab (reusing #14), with a fixed seed so output stays deterministic.
- Add `--save-palette out.csv` (Lospec-compatible) so a generated palette can be reused across a batch for consistent colours.
- Without this, Dither can't answer "reduce this photo to 16 good colours", which pngquant, ImageMagick and Ditherista all handle. It comes after Phases 1–2 only because it is a larger piece of work.

---

## Phase 4: Algorithm depth

### 17. Riemersma and variable error diffusion — Medium / M
- **Riemersma:** error diffusion along a Hilbert curve with a decaying error history. This is a new `ScanPattern` value (`HILBERT`) plus a history buffer.
- **Ostromoukhov / Zhou–Fang:** coefficients that vary with intensity. These are presets whose taps are looked up per pixel, so `ErrorDiffusion` needs to support taps that depend on intensity.

### 18. Dot diffusion and Direct Binary Search — Low / M–L
- Important to quality-focused users, but niche. DBS is slow, which suits the existing `--benchmark` flag and deterministic output. Do this after the features above.

---

## Phase 5: Longer-term bets

### 19. Library API — Medium / M
- Split everything except `main.cpp` into a static library with a small public header (`load`/`process`/`save` on memory buffers). The Makefile already builds `LIB_OBJ` for the tests, so most of the separation exists. Add a CMake build for people who embed it.

### 20. WebAssembly build and browser demo — High / M–L
- With few dependencies and the library from #19, Emscripten can compile Dither into a static web page with live preview. This answers the "GUI tools all have live preview" gap without writing a native GUI, and gives people a link to try Dither instantly. The didder author's paid GUI (Dithertime) suggests there is demand for a graphical companion.

### 21. Retro-hardware constraints — Medium / L
- Per-block colour limits (for example 8×8 cells with 2 colours for the ZX Spectrum, C64 multicolour modes and NES attribute tables), as Dithertron does. This is a strong fit for the game-developer audience, but it means a new constrained-quantisation stage.

### 22. SVG output — Low / M
- Merge runs of same-coloured pixels into rectangles or paths. Dither Boy offers this, but demand from command-line users is probably low.

### Out of scope for now
Video input and output, and After Effects plugins. This is where most of the paid market is, but it is a different product. Revisit after the WebAssembly demo shows whether there is interest.

---

## Suggested release grouping

| Release | Contents | Theme |
| :-- | :-- | :-- |
| **2.1** | #1–#3, #4, #5, #6 | "Easy to get, easy to feed": binaries, JPEG input, strength |
| **2.2** | #7–#10, #11 | Halftones, scaling, recolouring, GIF timing |
| **2.3** | #12–#15 | Pipelines: batch, pipes, OKLab, tone controls |
| **3.0** | #16 | Automatic palettes |
| **3.x** | #17–#22 | Algorithm depth, library, web demo |

Each release should ship with tests in `tests/test_main.cpp`, following the existing pattern (including `XFAIL` tracking for known gaps) and a changelog entry.
