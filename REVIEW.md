# Dither — Project Review

**Scope:** every first-party source file (`src/`, `include/` apart from the vendored lodepng, cxxopts and gif-h), the `Makefile`, `run.bat`, the `data/` CSVs and `README.md` (taken as the spec).
**Method:** I read all the code, then built the project (GCC 13, `-std=c++23`) and ran targeted tests against `input/boat.png` and small synthetic images to confirm each bug below. A finding marked **Verified** was reproduced by running the program. A finding marked *By inspection* comes from reading the code only.
**Revision:** updated 30 Oct 2026 against `main` at `2414105` (tag `v1.0.0`), plus the ROADMAP Phase 0–4 work. See §0 for what changed since the first review.

---

## 0. Changes since the first review

### 0.1 Changes in `v1.0.0`

| Change in the repo | Effect on this review |
|---|---|
| `--benchmark` implemented: new `include/benchmark.h` and `src/benchmark.cpp` (a `steady_clock` timer), wired into `main.cpp` | **S1 resolved.** I rebuilt and ran it: it prints e.g. `processing time: 81ms`. A few minor notes are added as Q11. |
| `Makefile` `_DEPS`/`_OBJ` now include `benchmark` | Builds correctly. The four headers that were already missing from `_DEPS` are still missing (§5). |
| Example images for the README commands removed from `output/` (all of the `*ordered*`, `*error_diffusion*`, `*temporal*`, `*convolve*`, `*reduce*` and `*grayscale*` files) | The defective ordered-dither examples (B1) are gone. The README examples now have no reference outputs. The `make clean` note in §5 changes to match. |
| New `input/david.png`; tag `v1.0.0`; new local branch `ai` | No effect on the findings. |
| `README.md` and every other source file | **Unchanged.** Every bug (B1–B6, Q1–Q10) and every remaining spec gap (S2–S20) still applies. `bin/dither.exe` is still the 20 Oct build. |

### 0.2 ROADMAP Phase 0 (30 Oct 2026, not yet committed)

| Change | Effect on this review |
|---|---|
| New Makefile (saved as `Makefile.new`, because the remote tools can't overwrite `Makefile`): C++23, `-O2 -Wall -Wextra -Wconversion`, automatic `-MMD -MP` dependencies, real file targets (`.exe` on Windows), `all` / `test` / non-destructive `clean`, warnings off for vendored lodepng and gif-h | Resolves most of §5 once `Makefile.new` replaces `Makefile`. Builds cleanly on GCC 13; not yet tried on MinGW. |
| Missing includes added to `ordered.h`, `convolve.h` and `benchmark.h` | **Q7 resolved**, along with the `<cstddef>` and stale-comment parts of Q11. |
| New `tests/test_main.cpp`: 9 regression tests, run with `make test` | **§2.3 point 7 resolved.** 1 test passes and 8 fail as expected, one per bug B1–B6, in 0.3 s. These tests now count as the reproduction for each bug (see §3). |
| `.gitignore` also ignores `*.d`, `bin/dither`, `bin/test_dither` and `tests/tmp/` | — |
| Running the stricter warnings found something new | **Q12 added.** Also corrects §5: `-Wconversion` does **not** flag B1; only `-Wsign-conversion` does. |

### 0.3 ROADMAP Phase 2 (30 Oct 2026, not yet committed)

| Change | Effect on this review |
|---|---|
| `Dither::convolve` rewritten: signed indices, clamp-to-edge sampling, flipped kernel, rounding, alpha kept; `_convolve` template removed | **B2 resolved**, including the correlation-vs-convolution and truncation notes. Convolution now also keeps alpha (part of Q2). |
| `convolve_identity` is now a `TEST`; new `convolve_blur_solid`, `convolve_is_true_convolution` and `convolve_keeps_alpha_and_rounds` | `make test`: 11 pass, 1 expected failure (B3). |
| `-Wtype-limits` warnings from the old padding loops are gone | 52 first-party warnings on a clean build, down from 64. |

### 0.4 ROADMAP Phase 3 (30 Oct 2026, not yet committed)

| Change | Effect on this review |
|---|---|
| Float working buffer (`RGBf`) for reduce, error diffusion, ordered and temporal; only the chosen palette colour's original sRGB value is written out | **B3 resolved**, with **Q1** (error carried in float) and **Q6** (distances in `double`) on every dither path. §2.3 point 5 resolved. |
| New `Transfer`: standard sRGB curve (÷255, exponent 2.4); `gAMA` used as a pure power curve only without an `sRGB` chunk; palette linearised once | `-a` now gives a white fraction of 0.275 (FS) / 0.273 (`BAYER_8X8`) on `boat.png`, against the 0.273 target. All 256 levels round-trip. `-g BT709 -a` leaves a grey ramp unchanged and keeps ≥ 250 levels of a colour gradient (190 before). |
| Thresholds replaced by their centred mid-rank in the matrix | **Q3 resolved**, and the small white-noise matrices are no longer biased either. |
| Transparent pixels pass through, spread and receive no error; alpha kept | **Q2 resolved** for every operation (decision D3). |
| Raster and serpentine loops merged with signed coordinates | **Q12 resolved**; the raster/serpentine part of §2.3 point 6 is done. |
| Removed `Color::to_linear`/`to_srgb`, `Image::get_color_range`, `Palette::get_color_range`, `Palette::range_slope` | The `get_color_range` duplication in §2.3 point 6 is gone. |
| `gamma_linear_coverage` is now a `TEST`; six new tests (`transfer_round_trip`, `png_gamma_chunks`, `grayscale_linear_levels`, `ordered_all_matrices_coverage`, `error_diffusion_keeps_small_error`, `alpha_preserved`) | `make test`: 18 pass, 0 expected failures, 0.4 s. Each new test fails on the Phase 2 code. |
| Clean build | 8 first-party warnings, down from 52. |

### 0.5 ROADMAP Phase 4 (30 Oct 2026)

Phase 4 is a refactor only: 861 of 861 Phase 3 outputs are byte-identical. ROADMAP "Phase 4 as built" has the details.

| Change | Effect on this review |
|---|---|
| `MappingMethod`, `ScanPattern` and `TemporalMethod` enums (`options.h`), parsed once in `CLI::parse` | **§2.3 point 1 resolved.** No string is compared per pixel any more, and the error-diffusion algorithm is also checked before the image is loaded. |
| Preset tables are function-local statics in `.cpp` files; Bayer matrices are generated in code; noise matrices are byte arrays in `src/ordered_tables.cpp` | **§2.3 point 3 resolved.** `dither.o` is 48 KB (1.75 MB before), `bin/dither` 0.8 MB (2.1 MB), and runs are 4–11 ms faster. |
| `DitherError` thrown everywhere below `main`; `main` prints it and returns 1 | **§2.3 point 4 resolved.** The same exit code for every bad input; the library errors are now also tested in-process. |
| One `csv::read_matrix` for kernels, threshold matrices and grayscale weights | **The CSV-parser part of §2.3 point 6 resolved.** Blank lines and CRLF are accepted, and fractional threshold values are rejected. |
| Old `Color` distance functions removed; `std::array` weights; `const&` everywhere; `Color::CHANNEL_MAX` is `int` | **Q5 resolved**, and so is the rest of Q6. |
| `-b` reports `load`, each operation, `save`, `processing` and `total`, only after a successful save | **Q11 resolved**, except for the README text, which is Phase 6. |
| `#pragma once` instead of `__NAME_H` guards | **Q8 resolved.** |
| `-Wsign-conversion` turned on (in `Makefile.new` until it replaces `Makefile`, which the remote tools can't write) | A clean build gives **0** first-party warnings (8 before). |
| 5 new tests, plus `ed_kernel_sums` rewritten | `make test`: 23 pass, 0 expected failures, under 1 s. The runner now reports an escaped exception as a failure instead of aborting. |
| The Phase 0 Makefile has replaced `Makefile` | §5 updated. |

---

## 1. Executive summary

The project is small and easy to follow, and its features line up with the README closely. The `Dither` facade, the lookup-table classes and the CSV extension points are good ideas. Most of the error-diffusion kernels, palettes and grayscale weights are transcribed correctly.

There are, however, several correctness bugs that change the output images. All of the ones below are verified.

| # | Severity | Finding |
|---|---|---|
| B1 | **Critical** | **Ordered dithering is broken for every threshold value below 127.** An `int × size_t` expression underflows, so about half the threshold cells always map to the darkest palette colour. Every `-d` output so far has this defect, including the README example images that have since been removed. |
| B2 | **High** ✅ | **Convolution shifts the image** right and down by half the kernel size and leaves black borders on the top and left edges. Edge padding is never written. |
| B3 | **High** ✅ | **Gamma correction (`-a`) darkens dithered output heavily.** *(Resolved in Phase 3.)* Pixels are linearised but palette colours are not. There is also 8-bit truncation in linear space. |
| B4 | **High** | **`UNIFORM_HISTOGRAM` is wrong in three ways:** it uses the red slope for all channels, it computes the colour range incorrectly, and it divides by zero when a channel is flat. |
| B5 | Medium | **`WHITE_NOISE_64X64` cannot be used.** The table key is misspelled `WHITE_NOISE_64X46`. |
| B6 | Medium | **Several inputs crash the program or fail silently:** a non-numeric frame count, an empty palette, an unknown output extension, 0 frames, and invalid temporal or scan options. |

On the spec side, the main gaps are listed below. (`--benchmark`, a gap in the first review, is now implemented.)
- Several syntax lines in the README are out of date.
- `UNIFORM_HISTOGRAM` doesn't do what the README describes.
- `EUCLIDEAN_DISTANCE` is actually luma-weighted.
- The README claim that 8×8 Bayer "contains every possible value (0–255)" is incorrect.

---

## 2. Architecture

### 2.1 Structure

```
main.cpp                          catches DitherError (error.h), prints it, returns 1
  └─ CLI (cli.cpp, cxxopts)        parses flags; enum-like values -> enums (options.h)
  └─ Benchmark (benchmark.cpp)     named steps: load, each operation, save (-b)
  └─ Dither (dither.cpp)           facade: owns one Image, one method per operation
        ├─ Grayscale   (grayscale.cpp)       preset weights or CSV  → std::array<double, 3>
        ├─ Convolve    (convolve.cpp)        preset kernels or CSV  → 2-D vector<double>
        ├─ ErrorDiffusion (error_diffusion.cpp) {dx, dy, scalar} taps
        ├─ Ordered     (ordered.cpp)         generated Bayer, byte-array noise matrices (ordered_tables.cpp) or CSV
        ├─ csv::read_matrix (csv.cpp)        the one CSV matrix parser for the three above
        ├─ Palette     (palette.cpp)         preset palettes or Lospec CSV; nearest-colour search
        ├─ Color / RGBf / Transfer (color.cpp) 8-bit colour, float working colour, sRGB <-> linear
        └─ Image       (image.cpp)           vector of RGBA8 frames; PNG via lodepng, GIF via gif-h
```

`main` runs the operations in a fixed order, whatever order the flags were given in: grayscale → convolve → reduce → error diffusion → ordered → temporal. It then saves the result.

### 2.2 Strengths
- The modules are split along clear lines: CLI parsing, image I/O, colour maths and algorithms each live in their own place.
- The same pattern is used everywhere: a built-in preset by name, or a CSV path to extend it. This is consistent and easy for users to learn.
- Error-diffusion kernels are data (coordinates and scalars) rather than hand-written loops, which makes new kernels cheap to add.
- Accepting Lospec CSVs directly is a nice touch.

### 2.3 Architectural concerns

1. ✅ *(Resolved in Phase 4: enums parsed once in `CLI`.)* **Configuration is passed around as strings.** Method names travel as `std::string` all the way down to the per-pixel code. For example, `Palette::nearest` compares `mapping_method` against string literals for every pixel, and nothing is validated up front. Because of this, invalid values are caught late or not at all (see B6). Parsing into `enum class`es once, in `CLI`, would fix both problems.
2. **The CLI model has one shared slot for the mapping method and palette.** `-r`, `-e`, `-d` and `-t` all write into the same `mapping_method` and `palette` fields (`cli.cpp:107-108, 124-125, 140-141, 157-158`). If you combine operations, the last flag silently wins for all of them. The README says "any number of optional flags" but never says how combinations behave.
3. ✅ *(Resolved in Phase 4: static tables in `.cpp` files; generated Bayer; byte-array noise matrices.)* **Lookup tables are rebuilt on every construction.** `PRESET_PALETTES`, `KERNELS`, `METHODS`, `COORDINATES`, `SCALARS` and `THRESHOLD_MATRIXES` are non-static `const` data members. Each `Ordered` object therefore rebuilds an `unordered_map` holding all 20 matrices, including two 256×256 ones. Putting ~600 KB of numbers in `ordered.h` also makes compiles slow: `dither.o` is 3 MB. These tables should be `static inline const` data, ideally in a `.cpp` file, or loaded from the CSVs that already exist in `data/threshold_matrixes/`.
4. ✅ *(Resolved in Phase 4: `DitherError`.)* **Library code exits the process.** `std::exit(EXIT_FAILURE)` is called from constructors and from `Palette::nearest`. That makes the classes impossible to reuse or test. Returning errors, or throwing and catching in `main`, would be cleaner.
5. ✅ *(Resolved in Phase 3: dithering uses a float working buffer.)* **The colour pipeline is 8-bit integers throughout**, including linear-light values and diffused error. This causes avoidable precision loss (B3, Q3). A `float` working buffer is the usual approach.
6. ✅ *(Resolved: the slope set-up in Phase 1, the scan loops and `get_color_range` in Phase 3, and the CSV parser in Phase 4.)* **Code is duplicated in several places:**
   - The `UNIFORM_HISTOGRAM` slope set-up is copied three times in `dither.cpp`.
   - The raster and serpentine loops are near-identical 40-line blocks.
   - The CSV matrix parser is copied between `convolve.h` and `ordered.h`.
   - `get_color_range` is copied between `Image` and `Palette`, along with its bug.
7. ~~**There are no tests.**~~ ✅ **Resolved in Phase 0.** `tests/test_main.cpp` has 9 regression tests (`make test`). They are golden-image and property checks: for example, an identity kernel must give back the same image, and greys at 25%, 50% and 75% ordered-dithered to two colours must come out 25%, 50% and 75% light. Bugs that aren't fixed yet are marked as expected failures, and the runner reports when one starts passing.

---

## 3. Bugs

### B1 — Ordered dithering: negative offsets overflow (Critical, **Verified**)
`dither.cpp:342-346`
```cpp
threshold_value_scaled * palette_pitch_vector.r / Color::CHANNEL_MAX
```
`Color::CHANNEL_MAX` is a `std::size_t`. When `threshold_value_scaled` is negative, the whole product is converted to unsigned. Dividing then gives about 16,842,906, not about −102. That colour is then passed to `distance_euclidean_squared`, which overflows `int` and returns `INT_MIN` for every palette colour, so index 0 (the darkest colour) always wins.

**Effect:** every threshold cell below 127 (half of every matrix) forces the darkest colour, whatever the pixel is.

**Evidence:** I emulated the overflow in Python. It predicts a white fraction of 0.397 for `boat.png` with `BAYER_8X8` and `1BIT_MONITOR_GLOW`, and the binary gives exactly 0.397. The correct algorithm gives 0.452. A one-pixel test shows offset −1 producing `offset.r = 16843008`.

**Test:** `ordered_grey_coverage`. A 75% grey comes out 50% light. A 50% grey passes even with the bug, because forcing the darkest colour on every negative-offset cell happens to be correct at the exact midpoint, so a midpoint-only test can't catch B1.

**Fix:** `static_cast<int>(Color::CHANNEL_MAX)` here, or make the `Color` constants `int`. The defective `output/*ordered*` example images have already been removed. If you restore examples, generate them after this fix.

### B2 — Convolution shifts the image and never pads edges (High, **Verified**) — ✅ resolved in Phase 2
`dither.cpp:63-130`, `dither.h:55-56`
- The top and left padding loops are `for(std::size_t y = -kernel_height_half; y < 0; y++)`. An unsigned value is never `< 0`, so these loops never run. `-Wextra` warns about this.
- The bottom padding writes to row `y` rather than `y + kernel_height_half`. The centre fill then overwrites it, so the real bottom padding stays 0.
- The right padding writes to column `x` rather than `x + kernel_width_half`. That overwrites the last `kernel_width_half` real pixels of each row with the edge colour, and the real right padding stays 0.
- The result is read back as `convolved[y][x]` rather than `convolved[y + khh][x + kwh]`, which shifts the output by (kwh, khh).
- In `_convolve`, `(my + ky - kernel_height_half) % matrix_height` underflows as `size_t`. The wrap-around indices it produces are wrong (2⁶⁴ mod H ≠ 0), although they stay in bounds.

**Evidence:** a 3×3 identity kernel gives an image that matches the source shifted by (1,1) at 100% of pixels. The top row and left column come out pure black (mean 0.0, against 148.7 and 119.8 in the source). A 5×5 identity kernel gives a (2,2) shift.

**Test:** `convolve_identity`. 186 of 187 pixels change with 3×3, 5×5 and 3×5 identity kernels.

**Fix:** use signed indices with clamp-to-edge sampling directly inside `_convolve`. This removes the need for a padded copy at all.

**Also:**
- The code computes correlation, not convolution: the kernel is never flipped. That makes no difference for the symmetric built-in kernels, but it mirrors asymmetric user kernels such as `emboss.csv`.
- `static_cast<T>(sum)` truncates toward zero instead of rounding.

### B3 — Gamma correction produces output that is too dark (High, **Verified**) — ✅ resolved in Phase 3
`color.cpp:77-109`, `dither.cpp:163-168, 211-226, 333-348, 380-383`
1. **Palette colours stay in sRGB.** In `reduce`, `error_diffusion`, `ordered` and `temporal`, the pixel is converted to linear and then compared against sRGB palette colours. The error is also computed against those sRGB values.
2. **`to_linear` and `to_srgb` store linear values as 8-bit `int`.** sRGB values 0–10 all become 0, and only 190 of 256 levels survive a round trip. This is also why `-g BT709 -a` drops from 203 to 163 distinct grey levels.
3. **Scaling is off.** The code divides by 256 (`CHANNEL_VALUES`) rather than 255, and it uses the PNG's `gAMA` value (default 2.2) as the exponent inside the sRGB piecewise formula, which is defined with 2.4. With this mix, 255 maps to 253 linear and comes back as 254.

**Evidence:** `boat.png` error-diffused with Floyd–Steinberg to `1BIT_MONITOR_GLOW`:

| Run | White fraction |
|---|---|
| Without `-a` | 0.452 (correct for sRGB-space dithering) |
| With `-a` (current code) | 0.193 |
| Correct linear-light dithering | 0.273 (computed) |

`-r ... -a` gives mean luma 44 against 151 without `-a`.

**Test:** `gamma_linear_coverage` (white fraction 0.193 against a target of 0.273).

**Fix:**
- Keep a float linear working buffer.
- Linearise the palette once.
- Do distances and error in linear space, then output the palette's original sRGB colour.
- Use the standard sRGB transfer function (2.4 exponent, divide by 255). Honour `gAMA` only by converting through it properly.

### B4 — `UNIFORM_HISTOGRAM` mapping (High, **Verified**)
1. `palette.cpp:127-129` uses `slope[Color::INDEX_R]` for the G and B channels too (copy and paste).
2. `image.cpp:125-150` and `palette.cpp:233-258` find the minimum and maximum with `if … else if` and start from min=255, max=0. If the first value seen sets the minimum, the maximum is never updated for it. For data in descending order, the maximum stays 0, below the minimum, and the slope goes negative.
   - **Evidence:** a 2-pixel image [200, 100] maps both pixels to black, while [100, 200] maps them correctly to black and white.
3. If a channel has the same value everywhere, the slope divides by zero (`dither.cpp:151-153` and its two copies).
   - **Evidence:** a solid (100,150,200) image maps to the darkest colour `#292831`. `EUCLIDEAN_DISTANCE` gives `#4a7a96`.
4. For the spec gap, see S5.

**Tests:** `uniform_hist_order_independent` and `uniform_hist_flat_channel`. The second expects a solid image to give the same result as `EUCLIDEAN_DISTANCE`, so the fix should leave a flat channel unmapped rather than mapping it to the palette minimum. ROADMAP 1.3 has been changed to match.

### B5 — `WHITE_NOISE_64X64` is unreachable (Medium, **Verified**)
`ordered.h:657` (line 652 before Phase 0 added includes) has the key `"WHITE_NOISE_64X46"`. Passing `WHITE_NOISE_64X64`, as documented, falls through to the CSV path and fails with `Error: invalid threshold matrix - WHITE_NOISE_64X64`.

**Tests:** `preset_names_resolve`, which reads every preset name from the README tables, and `presets_match_csv`. All the other 19 built-in matrices match their CSVs exactly.

### B6 — Crashes, undefined behaviour and silent failures (Medium, **Verified** unless noted)

| Input | Location | Result |
|---|---|---|
| `-r TWILIGHT5` (1 argument) | `cli.cpp:102` | The error message reads `[1]` out of bounds and prints garbage memory |
| `-t PWM,abc,…` | `main.cpp:52` | Uncaught `std::invalid_argument`; `abort()` with exit code 134 |
| `-t PWM,-1,…` | `main.cpp:52` | Negative value converted to `size_t`, a huge allocation (*by inspection*) |
| `-t PWM,0,…` | `image.cpp:205-221` | No file written, exit code 0 |
| `-o out.jpg` | `image.cpp:255-258` | Prints an error but returns 0, so the exit code is 0 and no file is written |
| Palette CSV with no colours | `palette.cpp:76, 149` | `colors_size - 1` underflows in `sort`, and `colors[-1]` is read: segfault |
| 1-colour palette with `-d` | `palette.cpp:212` | Divides by `colors_size - 1 = 0`; converting `inf` to `int` is undefined behaviour (*didn't crash in testing*) |
| `-t PWM,…,UNIFORM_HISTOGRAM,…` | `dither.cpp:407-422` | `distance_darker` and `distance_lighter` are read uninitialised; GCC warns about this |
| `-t RANDOM,…,FOO,…` | `dither.cpp:457-469` | `weights` stays empty, so every pixel silently becomes `palette[0]`, exit code 0 |
| `-e …,SNAKE,…` (bad scan pattern) | `dither.cpp:204` | Silently treated as `RASTER` |
| `RANDOM` with a pixel equal to a palette colour | `dither.cpp:461` | `1.0 / 0` gives an `inf` weight. libstdc++ happened to handle it, but it is fragile (*by inspection*) |
| Palette CSV empty, or line 1 empty | `palette.cpp:33` | `tokens[0]` read out of bounds (*by inspection*) |
| GIF write | `image.cpp:236-241` | Only the last frame's result is checked, and a `GifBegin` failure is ignored |
| Threshold CSV values outside 0–255 | `ordered.h:1176` | Not validated, although the README says they must be. Confirmed by test: a value of 300 is accepted, exit 0 |

**Test:** `cli_rejects_bad_input` runs 13 of these inputs against the real binary. On Linux, two abort (exit 134), two segfault (139) and seven exit 0 when they should fail. Two cases already behave correctly: a 1-colour palette with `-d` gives a correct image, and a GIF written into a missing folder fails with an error. The `-r TWILIGHT5` garbage read did not reproduce on Linux, so it is checked only as "output is printable".

### Smaller correctness and quality issues
- ✅ *(Phase 3)* **Q1 — Error is truncated, not carried.** `static_cast<int>(error * scalar)` at `dither.cpp:240-242, 284-286` truncates toward zero, so small errors vanish. The effect is negligible with high-contrast palettes (measured within 0.2%), but it causes banding with close palettes. A float error buffer fixes it.
- ✅ *(Phase 3, D3)* **Q2 — Alpha is discarded.** Every operation writes alpha = 255, and transparent pixels get dithered and pass error on to their neighbours. A fully transparent test image came out opaque. The README says nothing about alpha either way.
- ✅ *(Phase 3: mid-rank centring, which also fixes the uneven small white-noise matrices)* **Q3 — Bayer thresholds are biased.** Built-in values run `k·256/N²` (2×2 is {0, 64, 128, 192}). Subtracting 127 gives a mean offset of −31 for 2×2, which darkens results slightly. Centred thresholds `(k+0.5)·256/N² − 128` remove the bias.
- **Q4 — PWM is stochastic, not pulse-width modulation.** Each frame draws a random number against the duty cycle. With 4–8 frames the real proportion varies a lot from pixel to pixel, which gives flicker noise. A deterministic schedule (lighter for `round(duty·frames)` frames, phase-shifted per pixel by a blue-noise value) would match the name and look much smoother. PWM also picks its darker/lighter pair using HSL lightness only, so hue is ignored.
- ✅ *(Phase 3 and 4)* **Q5 — Per-pixel copies.** `Palette::nearest(Color, std::string, std::vector<Color>, std::vector<Color>, std::vector<double>)` takes everything by value, so each pixel copies a string and three vectors. `distance_euclidean_squared` also allocates its default weight vector on every call. Passing by `const&` and using `std::array` would fix this. Speed is fine at `-O1` (~0.14 s for 512×512). The old Makefile built at `-O0`; the Phase 0 Makefile uses `-O2`.
- ✅ *(Phase 3: dither paths use `RGBf` distances in `double`; the old `Color` functions remain but are unused there)* **Q6 — Weighted distance truncated.** `distance_euclidean_squared` returns `int` from a weighted `double` sum, which truncates. It also overflows for out-of-range inputs, and that overflow is what turns B1 into a silent wrong answer rather than a merely bad one.
- **Q7 — Fragile includes.** `ordered.h` uses `std::ifstream`, `std::stringstream`, `std::filesystem` and `std::count_if` without including `<fstream>`, `<sstream>`, `<filesystem>` or `<algorithm>`. `convolve.h` is missing `<algorithm>`. It builds on your MinGW toolchain only through transitive includes. **On GCC 13/Linux it fails:** `'count_if' is not a member of 'std'`. ✅ **Resolved in Phase 0**: `ordered.h` and `convolve.h` now include `<algorithm>`, `<fstream>`, `<sstream>`, `<filesystem>` and `<iterator>`, and the project builds on GCC 13.
- ✅ *(Phase 4)* **Q8 — Reserved names.** Header guards such as `__CLI_H` use reserved identifiers. Prefer `DITHER_CLI_H` or `#pragma once`.
- **Q9 — Wrong help text.** The help for `-g` says "Applies gamma correction the the input image" (`cli.cpp:41`). The `-a` text has the same "the the" typo.
- **Q10 — Unclear extension check.** `save` decides the format with `contains(".png")`, so `my.png.backup.gif` is saved as a PNG. Check the extension instead.
- ✅ *(Phase 4, apart from the README text)* **Q11 — Benchmark details** (new code, *by inspection*, all minor):
  - The timer covers only the processing step, not load or save. That is a reasonable choice, but neither `--help` nor the README says so.
  - There is no breakdown per operation when several are combined.
  - The time is printed before `save()`, so it is still reported when saving fails (for example `-o out.jpg` prints a time, then the format error, then exits 0).
  - Running `-b` with no operation prints a time of about 400 ns. A message such as "no operations" would be clearer.
  - ~~The `#include <chrono>` comment still says `clock_t, clock()`, and `std::size_t` is used without `<cstddef>`.~~ ✅ Fixed in Phase 0.
  - The accessor methods could be `const`, and `benchmark.cpp` mixes tabs and spaces.
- ✅ *(Phase 3: loops merged with signed coordinates)* **Q12 — Error-diffusion bounds checks are dead code** (new in Phase 0, found by `-Wtype-limits`). At `dither.cpp:235` and `279`, `new_x` and `new_y` are `size_t`, so `new_x < 0` and `new_y < 0` can never be true. Today the result is still correct only because a negative offset wraps around to a huge value, which then fails the `>= image_width` / `>= image_height` test. It is fragile rather than wrong: use signed `int` coordinates when the raster and serpentine loops are merged (ROADMAP 4.4).

---

## 4. Spec (README) vs implementation

| # | README says | Implementation |
|---|---|---|
| S1 | `-b, --benchmark` "Displays benchmark information to stdout" | ✅ **Resolved in `v1.0.0`.** Prints the processing time in ns, µs or ms. The README could say what is timed (Q11). |
| S2 | Error diffusion syntax: `-e ALGORITHM,MAPPING_METHOD,PALETTE` (README line 44) | Needs 4 arguments: `ALGORITHM,SCAN_PATTERN,MAPPING_METHOD,PALETTE`. The CLI table at line 19 is correct; the section text is stale. `run.bat` also still uses the 3-argument form and fails. |
| S3 | Flag spelled `-e, --error-diffusion` (line 43) | The real long flag is `--error_diffusion`. The hyphenated form gives "Option 'error-diffusion' does not exist". |
| S4 | Temporal syntax `dither -i INPUT -t …` (line 60), with no `-o` | `-o` is required, and the program exits with "No output image file path specified". |
| S5 | `UNIFORM_HISTOGRAM` "maps each pixel such that the output contains approximately equal amount of each color" | It is a per-channel min–max linear stretch of the image range onto the palette range, followed by nearest-colour Euclidean. There is no histogram involved, and it is buggy too (B4). Either implement histogram equalisation (CDF → palette rank) or change the description. |
| S6 | `UNIFORM_HISTOGRAM` "Not valid when using `-t` flag" | Not enforced: it reads uninitialised values (B6). It should be rejected with a clear error. |
| S7 | `EUCLIDEAN_DISTANCE` "nearest palette colour in Euclidean space" | Weighted by BT.709 luma coefficients (0.2126, 0.7152, 0.0722) by default (`color.h:36`). That is a reasonable choice but undocumented. Meanwhile `MANHATTAN_DISTANCE` is unweighted, so the two are inconsistent. |
| S8 | `MANHATTAN_DISTANCE` "nearest palette color in Cartesian space" | The wording is misleading; Euclidean distance is Cartesian too. Better: "sum of absolute per-channel differences (L1)". |
| S9 | `WHITE_NOISE_64X64` | Unreachable because of the misspelled key (B5). |
| S10 | "an 8×8 bayer matrix contains every possible value (0-255)" | An 8×8 matrix has 64 values (0, 4, 8, …, 252). Only 16×16 has all 256. The sentence and its conclusion should be revised. |
| S11 | `-a` "Applies gamma correction to the input image" | Applies only to grayscale, reduce, error diffusion, ordered and temporal; `-c` ignores it. ~~It also behaves incorrectly (B3).~~ Correct since Phase 3. The README should say what "gamma correction" means here: processing in linear light. |
| S12 | Convolution, reduce and dither sections: "input file and output files must be .png" | Not validated. `.gif` output works for every operation, and other extensions fail silently with exit code 0 (B6). |
| S13 | "Use `dither` followed by … any number of optional flags" | When operations are combined, they run in a fixed order and share one mapping method and palette (Architecture §2.3, point 2). This should be documented, or each operation should get its own arguments. |
| S14 | Temporal section: "See Temporal Dithering Methods for available **threshold matrixes**" | Copy-paste wording. Also, the Temporal Dithering Methods table has no intro sentence, unlike the other tables. |
| S15 | Ordered section: "In addition to the built-in **convolution kernels** below …" | Should say "threshold matrixes". |
| S16 | "Why Use Dither…" list ends with an empty bullet `- ` | Unfinished. Could mention temporal dithering, serpentine scanning and blue-noise matrices. |
| S17 | To Compile: "Requirements: GNU C++ compiler, make" | The new Makefile uses `-std=c++23` (it was `-std=c++26`, which needs GCC 14 or newer), so GCC 13 is now the minimum. The README should say so, and should also mention `make test`. |
| S18 | `JARVICE_JUDICE_NINKE` | The algorithm is Jarvis–Judice–Ninke. Consider accepting `JARVIS_JUDICE_NINKE`, keeping the old name as an alias. |
| S19 | Temporal `RANDOM`: "weighted based on color distance" | Accurate (inverse distance), but it is worth stating that weights are 1/d and that exact matches are a special case. |
| S20 | Temporal `PWM`: "modulates each pixel between the nearest lighter and darker palette colors" | "Nearest" is decided by HSL lightness only, and the modulation is random per frame (Q4). |

The items that do match the README: all 9 error-diffusion kernels (including Atkinson's 3/4 error), both scan patterns, the 8 grayscale weightings, the 7 convolution kernels (the Gaussian sums to 1.000; `UNSHARP_MASK` = 2δ − G), all 11 palettes, the remaining 19 threshold matrices, the Lospec CSV format, and GIF / vertical-sprite-sheet PNG output for temporal dithering.

---

## 5. Build and repository hygiene
Items marked ✅ are fixed by the Phase 0 Makefile, which has now replaced `Makefile`.

- ✅ **Header dependencies are missing.** `_DEPS` left out `convolve.h`, `error_diffusion.h`, `grayscale.h` and `ordered.h`. *Phase 0:* `-MMD -MP` dependency files replace `_DEPS`; touching `ordered.h` now rebuilds only `dither.o` and `main.o`.
- ✅ **The target name isn't the file it builds.** *Phase 0:* the targets are now `bin/dither` (`bin/dither.exe` on Windows) with `all` as the default; `make dither` still works.
- ✅ **No optimisation and few warnings.** *Phase 0:* `-O2 -Wall -Wextra -Wconversion`, with warnings off only for vendored lodepng and gif-h. A clean build shows 71 first-party warnings: `-Wtype-limits` (B2, Q12), `-Wmaybe-uninitialized` (B6, PWM) and many `size_t` → `unsigned`/`int` conversions. **Correction to the first review:** `-Wconversion` does *not* flag B1 on GCC 13. Only `-Wsign-conversion` does (`dither.cpp:343-345`). ✅ *Phase 4:* `-Wsign-conversion` is now on, and a clean build gives 0 first-party warnings.
- ✅ **`make clean` deleted everything in `output/`.** *Phase 0:* `clean` now removes only binaries, objects, `.d` files and `tests/tmp/`. The four images in `output/` (`eiffel_tower*.png`, `poker_table.png`, `will.png`) are safe. If example images come back, still keep them in a separate folder such as `docs/examples/`.
- **`bin/dither.exe` and `obj/*.o` are stale.** They are dated 20 Oct. Run `make clean` before the first build with the new Makefile: the old objects have no `.d` files, and some look newer than their sources, so make would otherwise keep them. Since then, `color.cpp`, `dither.cpp` and `palette.cpp` changed on 22 Oct, and `main.cpp` plus the new `benchmark.*` files arrived with `v1.0.0`. The binary on disk has no `--benchmark` support. The binary also sits in the tree even though `.gitignore` excludes `*.exe`.
- **`run.bat`** is git-ignored and points at another project (`scream_jam_2025`). It uses the obsolete 3-argument `-e` form. It's fine as a personal script, but don't mistake it for a project example.
- The `docs/` and `lib/` folders are empty. ✅ The unused `LDIR`/`LIBS` FFTW leftovers are removed from the Phase 0 Makefile.

---

## 6. Recommended fixes, in priority order
A step-by-step implementation plan is in **ROADMAP.md**.

1. **B1** — one-line cast fix in `ordered`. *(Phase 0 — build and tests — is done.)*
2. ~~**B2** — rewrite convolution with signed, clamped sampling; check it with an identity-kernel test.~~ ✅ done in Phase 2.
3. ~~**B3** — float linear pipeline, palette linearised once, standard sRGB transfer function.~~ ✅ done in Phase 3 (with Q1, Q2, Q3, Q6, Q12).
4. **B5, B6** — fix the key typo; validate every enum-like argument and the frame count in `CLI::parse`; return non-zero for unknown output formats; reject empty palettes.
5. **B4 / S5** — fix the slope and the min/max search, guard against division by zero, and decide whether to implement real histogram mapping or rename it (for example `RANGE_STRETCH`).
6. **Docs** — S2–S4 and S10–S17; document what `--benchmark` measures; Q11 tidy-ups.
7. ~~**Structure** — enums instead of strings, static tables, a float error buffer~~ ✅ done in Phases 3 and 4. Still to do: per-operation palette arguments (D2, Phase 5).
8. ~~**Build** — complete the dependencies, `-O2 -Wextra -Wconversion`, a non-destructive `clean`, the missing `#include`s~~ ✅ done in Phase 0. Still to do: the minimum GCC version in the README (S17).
