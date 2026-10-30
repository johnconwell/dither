# Dither — Roadmap

This is a plan for fixing the bugs and closing the spec gaps listed in **REVIEW.md**, written against `v1.0.0` (`2414105`). The IDs B*, Q* and S* refer to findings in that file.

The work is ordered so that:
1. a test harness exists before anything is changed;
2. cheap fixes that change the output ship first;
3. the bigger refactors come later, once tests protect them.

Each task lists the files it touches, the approach, and a **Done when** check that can be tested.

---

## Status

| Phase | State | Notes |
|---|---|---|
| 0.1 Build fixes | ✅ Done (30 Oct 2026) | Verified on GCC 13 / Linux. Not yet verified on MinGW. The new Makefile has since replaced `Makefile`. |
| 0.2 Test harness | ✅ Done (30 Oct 2026) | `make test`: 1 pass, 8 expected failures, 0.3 s. |
| 1.1–1.4 Quick correctness wins | ✅ Done (30 Oct 2026) | `make test`: 7 pass, 2 expected failures (B2, B3). Verified on GCC 13 / Linux; not yet on MinGW. |
| 1.5 Release v1.0.1 | Not started | Rebuild `bin/dither.exe`, tag, CHANGELOG. |
| 2.1 Convolution rewrite | ✅ Done (30 Oct 2026) | `make test`: 11 pass, 1 expected failure (B3). Verified on GCC 13 / Linux; not yet on MinGW. |
| 3.1–3.4 Colour pipeline, gamma, thresholds, alpha | ✅ Done (30 Oct 2026) | `make test`: 18 pass, 0 expected failures. Verified on GCC 13 / Linux; not yet on MinGW. |
| 3.5 Release v1.1.0 | Not started | Regenerate the README examples into `docs/examples/`. |
| 4.1–4.6 Internal refactor | ✅ Done (30 Oct 2026) | `make test`: 23 pass, 0 expected failures, 0 first-party warnings (now with `-Wsign-conversion`). Output byte-identical to Phase 3 on 861 CLI runs. Verified on GCC 13 / Linux; not yet on MinGW. The remote tools can't write `Makefile`, so the Phase 4 Makefile is in `Makefile.new` (new object list, `-Wsign-conversion`). |
| 4.7 Release v1.2.0 | Not started | Tag and CHANGELOG (no output changes; see "Phase 4 as built"). |
| 5.1–5.5 Spec decisions | ✅ Done (30 Oct 2026) | Decisions D1 (option B), D2, D4 and D5 taken. `make test`: 28 pass, 0 expected failures, 0 first-party warnings. 420 of 424 Phase 4 outputs byte-identical; the other 4 are `-c` with `-a` (D4, intended). Verified on GCC 13 / Linux; not yet on MinGW. |
| 5.6 Release v2.0.0 | Not started | Tag and CHANGELOG; see "Phase 5 as built" for the user-visible changes. |
| 6 Documentation | ✅ Done (30 Oct 2026) | Every row of the Phase 6 table is done except the CHANGELOG link, which waits for a CHANGELOG (1.5 / 3.5 / 4.7 / 5.6). See "Phase 6 as built". |

REVIEW.md §0.2 lists what Phase 0 changed in the findings; new finding Q12 (dead bounds checks) came out of it.

---

## Release overview

| Release | Phases | Theme | CLI compatible with 1.0? |
|---|---|---|---|
| **v1.0.1** | 0, 1 | Build fixes, tests, one-line and validation fixes | Yes |
| **v1.1.0** | 2, 3 | Correct convolution and a correct colour / gamma pipeline | Yes (output changes are bug fixes) |
| **v1.2.0** | 4 | Internal refactor: enums, static tables, error handling, speed | Yes |
| **v2.0.0** | 5, 6 | Spec decisions (histogram mapping, PWM, one quantiser per run, `-a` for convolution) and a README rewrite | **No:** combining `-r`/`-e`/`-d`/`-t` is now an error, and `UNIFORM_HISTOGRAM`, `PWM` and `-c -a` produce different output. `JARVICE_JUDICE_NINKE` still works. |

---

## Phase 0 — Safety net (do first) ✅

### 0.1 Fix the build *(REVIEW §5, Q7)* ✅
- **Files:** `Makefile`, `include/ordered.h`, `include/convolve.h`, `include/benchmark.h`
- **Approach:**
  - Add the missing includes: `<fstream>`, `<sstream>`, `<filesystem>` and `<algorithm>` to `ordered.h`; `<algorithm>` to `convolve.h`; `<cstddef>` to `benchmark.h`.
  - Replace the hand-written `_DEPS` list with automatic dependency files:
    ```make
    CFLAGS = -I$(IDIR) -std=c++23 -O2 -Wall -Wextra -Wconversion -MMD -MP
    -include $(OBJ:.o=.d)
    all: $(BDIR)/dither
    $(BDIR)/dither: $(OBJ) ; $(CC) -o $@ $^ $(CFLAGS)
    clean: ; rm -f $(BDIR)/dither $(BDIR)/*.exe $(ODIR)/*.o $(ODIR)/*.d
    ```
  - Lower the standard from `-std=c++26` to `-std=c++23`, which is enough for `std::string::contains`, so GCC 13 and newer can build it.
  - Make `clean` stop deleting `output/`.
  - Remove the unused `LDIR`/`LIBS` lines and the commented-out FFTW lines.
- **Done when:** `make` builds with no errors on GCC 13 (Linux) and your MinGW toolchain, and touching any header rebuilds the files that include it. The new warnings (`-Wextra`, `-Wconversion`) will expose B2. That is expected, and Phase 1 fixes them.
- **As built:**
  - The Makefile appends `.exe` to the targets on Windows, so incremental builds work there too. `make dither` still works.
  - Warnings are turned off for the vendored `lodepng.cpp` and `gif.cpp` only.
  - `<iterator>` was also added to both headers (for `std::istreambuf_iterator`).
  - A clean build gives 71 first-party warnings: `-Wtype-limits` (B2 and the error-diffusion bounds checks), `-Wmaybe-uninitialized` (B6, PWM) and many `size_t → unsigned`/`int` conversions.
  - **Correction:** `-Wconversion` does **not** flag B1 on GCC 13. Only `-Wsign-conversion` does (at `dither.cpp:343-345`). It is left off for now because it is noisy; consider it after Phase 4.
  - **New finding (REVIEW Q12):** the error-diffusion bounds checks at `dither.cpp:235, 279` (`new_x < 0`, `new_y < 0` on `size_t`) can never be true. They only work because a negative offset wraps around to a huge value, which then fails the `>= width` check. Use signed `int` coordinates when 4.4 merges the scan loops.

### 0.2 Add a test harness *(REVIEW §2.3, point 7)* ✅
- **Files:** new `tests/test_main.cpp`, `Makefile` (`test` target). The test images are generated at run time into `tests/tmp/`, which is git-ignored, instead of being committed to `tests/images/`.
- **Approach:**
  - Write a dependency-free runner: a `CHECK(expr)` macro plus a list of test functions.
  - Link it against all objects except `main.o`, and call the `Dither` and `Palette` APIs directly.
  - Generate the test images in code (solid colours, gradients, 2-pixel images) so nothing large is committed.
  - Start with the regression tests below, marked as *expected to fail*, then flip each one as its fix lands.

| Test | Checks | Today |
|---|---|---|
| `ordered_grey_coverage` | 25%, 50% and 75% greys (measured between the two palette colours) → `1BIT_MONITOR_GLOW` via `BAYER_8X8` give 25 / 50 / 75 ± 2% light | 75% gives 50% (B1). *Changed from the original 50%-only spec: at the exact midpoint B1 happens to give the right answer, so a 50% test passes with the bug in place.* |
| `convolve_identity` | A 3×3, a 5×5 and a 3×5 identity kernel leave the image unchanged | 186 of 187 pixels changed (B2) |
| `gamma_linear_coverage` | `boat.png` → 2 colours with `-a` gives a white fraction within ±2% of the computed linear target (0.273) | 0.193 (B3) |
| `uniform_hist_order_independent` | [200, 100] and [100, 200] map to mirrored colours | Both black (B4) |
| `uniform_hist_flat_channel` | A solid image gives the same result as Euclidean mapping | Darkest colour (B4) |
| `preset_names_resolve` | Every name in the README tables constructs without exiting | `WHITE_NOISE_64X64` fails (B5) |
| `presets_match_csv` | Every built-in threshold matrix equals its `data/threshold_matrixes/*.csv` | `WHITE_NOISE_64X64` fails (B5); all others match |
| `ed_kernel_sums` | Kernel scalars sum to 1, or to 0.75 for Atkinson | Pass |
| `cli_rejects_bad_input` | The B6 inputs return non-zero with a clear message, with no crash | Several crash or exit 0 |

- **Done when:** `make test` runs in under 5 seconds and reports pass and fail counts.
- **As built:**
  - Tests marked `XFAIL(name, "reason")` are expected to fail. When one starts passing, the runner reports **XPASS** and the run fails until you change `XFAIL` to `TEST`.
  - `-v` shows details for expected failures, and test names can be passed to run only those tests.
  - The CLI tests run `bin/dither` as a child process, so crashes are reported rather than killing the runner.
  - `preset_names_resolve` reads the names from the README tables, so documentation drift is caught as well.
  - Checked by applying the B1 and B5 one-liners in a scratch copy: `ordered_grey_coverage`, `preset_names_resolve` and `presets_match_csv` all flipped to XPASS.
  - `cli_rejects_bad_input` covers 13 cases. Today two of them crash with abort, two segfault and seven exit 0. The `-r TWILIGHT5` garbage read did not reproduce on Linux, and the GIF-into-missing-folder and 1-colour `-d` cases already behave correctly.

---

## Phase 1 — Quick correctness wins → **v1.0.1**

### 1.1 Fix the ordered-dither overflow *(B1, Critical)*
- **File:** `src/dither.cpp:342-346`
- **Change:** do the arithmetic as signed or floating-point:
  ```cpp
  constexpr int CH_MAX = static_cast<int>(Color::CHANNEL_MAX);
  Color offset(threshold_value_scaled * palette_pitch_vector.r / CH_MAX, …);
  ```
  A better long-term fix is to make the `Color` constants `int` (or `inline constexpr int`) so that mixing signed and unsigned values can't happen anywhere. `-Wsign-conversion` will point out any other places (`-Wconversion` alone does not; see 0.1).
- **Done when:** `ordered_grey_coverage` passes, and `boat.png` with `BAYER_8X8` gives a white fraction of about 0.45 (it is 0.397 today).

### 1.2 Fix the threshold matrix key *(B5)*
- **File:** `include/ordered.h` (the `WHITE_NOISE_64X46` key, now at line 657) — rename it to `WHITE_NOISE_64X64`.
- **Done when:** `preset_names_resolve` and `presets_match_csv` pass.

### 1.3 Fix the `UNIFORM_HISTOGRAM` arithmetic *(B4, items 1–3)*
This is an arithmetic fix only. Phase 5 decides what the method should do.
- **Files:** `src/palette.cpp:127-129, 233-258`, `src/image.cpp:125-150`, `src/dither.cpp` (the three slope blocks)
- **Change:**
  - Use `slope[INDEX_G]` and `slope[INDEX_B]` for the G and B channels.
  - Replace `else if` with two independent `if`s when finding the minimum and maximum, or use `std::minmax`.
  - Move the slope calculation into one helper, `compute_range_slope(image_range, palette_range)`. When a channel's range is 0, that channel should be **left unmapped** (the input value passes straight through to the nearest-colour search). *Changed from "slope 0, map to the palette minimum", which conflicts with `uniform_hist_flat_channel`: that test expects a solid image to give the same result as `EUCLIDEAN_DISTANCE` (`#4a7a96`, not the darkest colour `#292831`).*
  - Call that helper from all three places.
- **Done when:** both `uniform_hist_*` tests pass.

### 1.4 Validate input up front *(B6, S6, S12)*
- **Files:** `src/cli.cpp`, `src/main.cpp`, `src/image.cpp`, `src/palette.cpp`, `src/dither.cpp`
- **Change:**
  - In `CLI::parse`:
    - Check `temporal_frames` with `std::from_chars`; require it to be at least 1 and at most a sensible cap (for example 256).
    - Check the scan pattern is `RASTER` or `SERPENTINE`, and the temporal method is `PWM` or `RANDOM`.
    - Reject `UNIFORM_HISTOGRAM` with `-t`.
    - Check the output extension is `.png` or `.gif` (case-insensitive), and allow `.gif` only with `-t`, or document that it is allowed everywhere.
    - Fix the out-of-bounds read in the `--reduce` error message (`cli.cpp:102`).
  - In `Palette`: error out if the CSV is empty or has no colours, and handle a 1-colour palette in `pitch_vector()` by returning a zero vector.
  - In `Image::save`: return non-zero for an unknown extension and when there are 0 frames. Check the result of `GifBegin` and of every frame (`ok = ok && GifWriteFrame(…)`).
  - In `temporal`, for `RANDOM`: if any distance is 0, give that palette colour 100% probability.
  - Fix the `-g` help text and the "the the" typos (Q9).
- **Done when:** `cli_rejects_bad_input` passes, and every row of the B6 table in REVIEW.md gives either a correct result or a non-zero exit with a clear message.

### Phase 1 as built (30 Oct 2026)
- **1.1 (B1):** a local `constexpr int CHANNEL_MAX` keeps the offset arithmetic signed. `boat.png` with `BAYER_8X8` → `1BIT_MONITOR_GLOW` now gives a white fraction of 0.452. Making the `Color` constants `int` is left for Phase 4.
- **1.2 (B5):** key renamed to `WHITE_NOISE_64X64`.
- **1.3 (B4):**
  - New `Color::expand_range()` (independent min/max per channel) replaces the duplicated `else if` blocks in `Image` and `Palette`.
  - New `Palette::range_slope()` replaces the three copied slope blocks in `dither.cpp`. A flat channel gets the slope `Palette::UNMAPPED` (NaN), and `Palette::nearest` passes that channel through unchanged.
  - The G and B channels now use their own slopes.
- **1.4 (B6, S6, S12, Q9, Q10):**
  - New private `CLI::validate()` runs at the end of `parse`. It checks:
    - the output extension (`.png` or `.gif`, case-insensitive; `.gif` stays allowed for every operation);
    - the scan pattern and the temporal method;
    - the mapping method, which must be Euclidean or Manhattan for `-t`, so `UNIFORM_HISTOGRAM` is rejected there;
    - the frame count, parsed with `std::from_chars` into a new `temporal_frame_count`, range 1–256.
  - `main` uses `temporal_frame_count` instead of `std::stoi`.
  - The garbage line in the `--reduce` error message is removed.
  - `Palette`: rejects a CSV with fewer than 3 fields (empty file or no colours) and strips a trailing `\r`. `pitch_vector()` returns zero for a 1-colour palette, and `sort()` no longer underflows.
  - `Image::save`:
    - decides the format by extension instead of `contains` (Q10);
    - returns non-zero for unknown formats and for 0 frames;
    - checks `GifBegin` and every `GifWriteFrame`;
    - writes its errors to stderr.
  - `temporal`: PWM distances are initialised and an unknown method exits with an error. For `RANDOM`, an exact palette match gets 100% probability instead of an `inf` weight.
  - Threshold CSV values outside 0–255 are rejected. This was planned for 4.4, but `cli_rejects_bad_input` needs it now.
  - Help text fixed for `-a` and `-g` (Q9). `CLI::to_string` was missing the `temporal:` label.
- **Tests:** the six fixed tests changed from `XFAIL` to `TEST`.
- **Regression check:** error diffusion (raster and serpentine, with and without `-a`), reduce and grayscale output is byte-identical to the previous build. Only ordered output changed, as intended.
- **Warnings:** a clean build now gives 64 first-party warnings, down from 71.

### 1.5 Release v1.0.1
- Rebuild `bin/dither.exe`, tag the release, and add a short CHANGELOG entry.

---

## Phase 2 — Convolution rewrite → part of **v1.1.0**

### 2.1 Clamp-to-edge convolution *(B2)*
- **Files:** `src/dither.cpp:50-133`, `include/dither.h:33-66`
- **Approach:** delete the padded-copy code. Write one function that samples the source directly using signed indices clamped to the image:
  ```cpp
  for y, x:                       // int
    acc = {0,0,0}
    for ky, kx:
      sy = clamp(y + ky - kh/2, 0, H-1); sx = clamp(x + kx - kw/2, 0, W-1);
      acc += src(sx, sy) * kernel[kh-1-ky][kw-1-kx];   // flipped = true convolution
    dst(x, y) = clamp(lround(acc), 0, 255)
  ```
  - Write into a separate output buffer so already-convolved pixels are never read again.
  - Keep the source alpha instead of forcing 255.
  - Flip the kernel so this is true convolution. That changes asymmetric kernels such as `emboss.csv`, so note it in the CHANGELOG.
- **Done when:** `convolve_identity` passes for 3×3, 5×5 and 3×5 kernels, and `BOX_BLUR` on a solid image gives back the same solid colour, edges included.

### Phase 2 as built (30 Oct 2026)
- **`Dither::convolve`** (`src/dither.cpp`) is rewritten as one pass with signed `int` indices:
  - it copies frame 0 into a source buffer first, so it never reads pixels it has already written;
  - each sample is clamped to the image (clamp-to-edge), so there is no padded copy and no black border;
  - the kernel is flipped in both directions, so this is true convolution;
  - each channel is rounded with `std::lround` and clamped to 0–255, instead of truncated;
  - the source alpha is kept instead of forced to 255.
- The `_convolve` template is removed from `include/dither.h`, and `<cmath>` is added there.
- **Tests:** `convolve_identity` changed from `XFAIL` to `TEST`. Three new tests, all of which fail on the old code:
  - `convolve_blur_solid` — `BOX_BLUR` and `GAUSSIAN_BLUR` on a solid image give back the same colour everywhere, edges included;
  - `convolve_is_true_convolution` — a 3×3 kernel with a single 1 in its top-left cell moves the image **up and left** by one pixel (a correlation would move it down and right);
  - `convolve_keeps_alpha_and_rounds` — alpha is kept, and a 1×1 kernel of 0.97 on value 10 gives 10, not 9.
- **Regression check on `boat.png`:**
  - error diffusion, ordered, reduce and grayscale output is byte-identical to the Phase 1 build;
  - with `BOX_BLUR`, `GAUSSIAN_BLUR`, `UNSHARP_MASK` and `RIDGE_4`, interior pixels match the old output, once you undo its shift, to within ±1 (rounding instead of truncation);
  - the top row now has a mean of about 148.7, the same as the source; it used to be 0–87, depending on the kernel.
- **Output change for users:** asymmetric user kernels such as `data/convolution_kernels/emboss.csv` now come out mirrored compared with v1.0.0, because the kernel is flipped. Note this in the CHANGELOG.
- **Warnings:** a clean build now gives 52 first-party warnings, down from 64.

---

## Phase 3 — Colour pipeline and gamma → **v1.1.0**

### 3.1 Float working buffer *(B3, Q1, Q6)*
- **Files:** `include/color.h`, `src/color.cpp`, `src/dither.cpp`, `src/palette.cpp`
- **Approach:**
  - Add a small `struct RGBf { float r, g, b; }` for working values.
  - Each dither operation does three steps:
    1. convert the image to `std::vector<RGBf>` once, linearising it if `-a` is set;
    2. do all the maths in float;
    3. write the chosen palette colour's **original sRGB** value to the output.
  - Error diffusion keeps a float error buffer, so nothing is lost to truncation (Q1).
  - Distances are computed in float, so the `int` overflow in Q6 can't happen.

### 3.2 Correct sRGB transfer and a linear palette *(B3)*
- **Files:** `src/color.cpp:77-109`, `src/palette.cpp`
- **Approach:**
  - Use the standard sRGB curve: `c/255`, threshold 0.04045, exponent **2.4**; the inverse uses 0.0031308 and 1/2.4. Precompute the forward direction as a 256-entry lookup table.
  - Only when a PNG has a `gAMA` chunk *and* no `sRGB` chunk, use a pure power curve `pow(c/255, file_gamma)` instead.
  - Store a linearised copy of the palette next to the sRGB copy, and use the linear copy for distance calculations and error when `-a` is set.
  - Grayscale with `-a`: compute in linear float and round only once at the end. This keeps all 256 grey levels, against 163 today.
- **Done when:**
  - `gamma_linear_coverage` passes;
  - converting `0..255` to linear and back gives the original value for all 256 levels;
  - `-g BT709 -a` on a gradient keeps at least 250 distinct grey levels.

### 3.3 Centre the threshold matrices *(Q3)*
- **File:** `src/dither.cpp` (ordered)
- **Change:** normalise with `(t + 0.5) / N_levels − 0.5`, where `N_levels` is the number of distinct matrix values (or 256 for CSVs), instead of `t − 127`. This removes the darkening bias of about −31 that the 2×2 matrix has.
- **Done when:** `ordered_grey_coverage` passes for **every** built-in matrix, not only 8×8.

### 3.4 Decide on alpha *(Q2)* — see open decision **D3**
- Simplest option: pixels with alpha 0 pass through unchanged, spread no error and receive no error; alpha is kept on output.

### Phase 3 as built (30 Oct 2026)
- **3.1 Float working buffer (B3, Q1, Q6):**
  - New `RGBf` in `color.h`: float channels on the 0–255 scale, with BT.709-weighted Euclidean, Manhattan and HSL-lightness helpers computed in `double`.
  - Every quantising operation (`reduce`, `error_diffusion`, `ordered`, `temporal`) now builds one working buffer of frame 0 (sRGB as float, or linear light with `-a`), does all its maths there, and writes the chosen palette colour's **original sRGB** value back.
  - Error diffusion adds error straight onto the float buffer, so small errors are carried instead of truncated (Q1). Distances are `double`, so the `int` overflow and truncation in Q6 can't happen on these paths.
  - The raster and serpentine loops are merged into one loop with `direction = ±1` and signed `int` coordinates, so the bounds check is real (Q12). This was planned for 4.4.
- **3.2 sRGB transfer and linear palette (B3):**
  - New `Transfer` class: the standard sRGB curve (÷255, 0.04045 / 2.4, inverse 0.0031308 / 1/2.4) with a 256-entry forward lookup table, rounding on the way back.
  - `Image::load` stores gamma = 0 (sRGB) unless the PNG has a `gAMA` chunk **and** no `sRGB` chunk; then the file's exponent is used as a pure power curve. The old default of 2.2 inside the sRGB formula is gone.
  - `Palette::set_working_space()` linearises the palette once. Sorting, nearest-colour search, the PWM darker/lighter search, the ordered pitch vector and the `UNIFORM_HISTOGRAM` range all use the working copy.
  - Grayscale with `-a` weights the channels in linear float and rounds once: a grey ramp is unchanged and a colour gradient keeps at least 250 grey levels (190 before). Without `-a` it is unchanged.
  - Removed the broken `Color::to_linear` / `to_srgb`, `Image::get_color_range`, `Palette::get_color_range` and `Palette::range_slope`. `UNIFORM_HISTOGRAM` now uses `Palette::RangeMap`, built from the working buffer (flat channels still pass through as `UNMAPPED`).
- **3.3 Centred thresholds (Q3) — changed from the plan:**
  - Each threshold is replaced by its **mid-rank** in its matrix: `(values below + half the equal values) / count − 0.5`.
  - For Bayer and blue-noise matrices (evenly spaced values) this is exactly `(k + 0.5)/N − 0.5`, as planned.
  - *Why not the planned `(t + 0.5)/N_levels − 0.5`:* the small white-noise matrices have uneven random values (`WHITE_NOISE_2X2` is {73, 140, 161, 170}), so a value-based formula can't meet "passes for every built-in matrix". `WHITE_NOISE_2X2` gave 0% light at 25% grey. Mid-rank makes every matrix unbiased while leaving the evenly spaced ones exactly as planned.
  - The offset is `threshold × pitch`, so each pixel moves up to half a palette step either way, in working space.
- **3.4 Alpha (Q2, decision D3 taken as recommended):** fully transparent pixels pass through unchanged, spread no error and receive none; every other pixel keeps its alpha. For `-t`, transparent pixels are copied into every frame.
- **Tests:** `gamma_linear_coverage` changed from `XFAIL` to `TEST` and now checks ordered dithering too. Six new tests, all of which fail on the Phase 2 code:
  - `transfer_round_trip` — all 256 levels round-trip for sRGB, γ 2.2 and γ 1.8; reference values (sRGB 128 → 21.586% linear);
  - `png_gamma_chunks` — no chunk / `gAMA` only / `gAMA` + `sRGB` give 0 / 2.2 / 0;
  - `grayscale_linear_levels` — grey ramp unchanged, colour gradient keeps ≥ 250 levels;
  - `ordered_all_matrices_coverage` — every built-in matrix gives 25/50/75 ± 2% light, and greys from 5% to 95% average 50 ± 2% (the old `BAYER_2X2` averaged 40%);
  - `error_diffusion_keeps_small_error` — a 1% grey gives about 1% light (the old code gave 0%);
  - `alpha_preserved` — for reduce, error diffusion, ordered and temporal.
- **Results on `boat.png` → `1BIT_MONITOR_GLOW` with `-a`:** Floyd–Steinberg white fraction 0.275 and `BAYER_8X8` 0.273, against an independently computed target of 0.273 (0.193 before).
- **Regression check against the Phase 2 build on `boat.png`:**
  - reduce (Euclidean and Manhattan), grayscale and convolution are byte-identical;
  - `UNIFORM_HISTOGRAM` reduce changes 1.7% of pixels (float mapping instead of truncation);
  - error diffusion changes 15–37% of pixels (dots move once error stops being truncated) with the same mean luma ± 1.3;
  - ordered `BAYER_8X8` changes 2% of pixels; `BAYER_2X2` mean luma goes from 109 to 128 (source 130), which is the bias removal.
- **Speed:** Floyd–Steinberg to `SLSO8` on `boat.png` drops from 76 ms to 25 ms, because the per-pixel string and vector copies are gone from the new search path.
- **Warnings:** a clean build now gives 8 first-party warnings (52 before); all are in `image.cpp` and the now-unused `Color::distance_euclidean_squared`.

### 3.5 Release v1.1.0
- Generate the README examples again with the fixed binary into `docs/examples/`, and link them from the README so people can see the difference.
- CHANGELOG: error-diffusion and ordered output changes (bug fixes), `-a` now correct, mirrored asymmetric kernels (Phase 2), alpha kept.

---

## Phase 4 — Internal refactor → **v1.2.0** ✅
None of these tasks should change any output. Prove that by comparing Phase 3 outputs byte for byte before and after.

### 4.1 Enums instead of strings *(REVIEW §2.3, point 1)*
- **Files:** `cli.h/.cpp`, `dither.h/.cpp`, `palette.h/.cpp`
- **Approach:**
  - Add `enum class MappingMethod`, `ScanPattern`, `TemporalMethod` and friends, each with a `parse(std::string_view) → std::optional<Enum>` function.
  - Parse once, in `CLI`, and pass enums down.
  - `Palette::nearest` then switches on the enum, with no string comparison per pixel.

### 4.2 Static lookup tables, moved out of the headers *(§2.3, point 3)*
- **Files:** `ordered.h`, `convolve.h`, `grayscale.h`, `error_diffusion.h`, `palette.h`
- **Approach:**
  - Turn the per-object `const` maps into `static const` data, defined once in `.cpp` files.
  - For the threshold matrices, choose one of two options:
    - **(a)** move the 646 KB of numbers into `src/ordered_tables.cpp`; or
    - **(b)** generate the Bayer matrices in code with the recursive formula, and embed only the noise matrices.

    Option (b) cuts about 40% of the data. Either way, `presets_match_csv` guards the values.
- **Done when:** `dither.o` shrinks noticeably from 3 MB and a clean build is faster.

### 4.3 Stop exiting from library code *(§2.3, point 4)*
- Constructors and loaders throw `DitherError` (or return `std::expected<T, std::string>`, which C++23 provides). `main` catches it, prints the message and returns `EXIT_FAILURE`. Nothing below `main` calls `std::exit`.
- Once this lands, the error cases in `cli_rejects_bad_input` can also be tested in-process.

### 4.4 Remove duplicated code *(§2.3, point 6)*
- ~~Combine the raster and serpentine loops into one loop with `dir = ±1` and `x0/x1`. Use signed `int` coordinates so the bounds check is real (REVIEW Q12).~~ ✅ Done in Phase 3.
- Share one CSV matrix parser between `Convolve` and `Ordered`. While doing so, validate the 0–255 range for threshold matrices and ignore blank trailing lines.
- Share one `color_range()` helper (already done in 1.3).

### 4.5 Speed *(Q5)*
- Pass by `const&` throughout `Palette::nearest`. *(Phase 3: the new `Palette::nearest_index` already does; the old `Color` distance functions are unused on the dither paths and can be removed.)*
- Replace the default `std::vector<double>` weights argument with a `static constexpr std::array`.
- Measure with `-b` before and after, using `boat.png` with Floyd–Steinberg to `SLSO8`.

### 4.6 Benchmark tidy-up *(Q11)*
- Print the time after `save()` and only if saving succeeded, or time load, processing and save separately.
- Optionally show the time for each operation.
- Make the accessors `const` and fix the stale include comment (done in 0.1).

### Phase 4 as built (30 Oct 2026)
**Output check:** 885 CLI runs (every preset, mapping method, scan pattern and threshold matrix, with and without `-a`, on `boat.png`, an RGBA image with transparent pixels and a PNG with a `gAMA` chunk, plus combined operations and GIF output). All 861 outputs the Phase 3 build produced are **byte-identical**. The other 18 runs used a threshold CSV with a trailing blank line, which Phase 3 rejected and Phase 4 accepts on purpose (4.4); they match the same matrix without the blank line. Temporal output is random, so only its exit codes were compared. The new `outputs_unchanged` test keeps 45 of these comparisons as pixel hashes taken from the Phase 3 build.

- **4.1 Enums (REVIEW §2.3 point 1):**
  - New `include/options.h`: `MappingMethod`, `ScanPattern` and `TemporalMethod`, plus `parse_enum<E>()`, `enum_name()` and `enum_names<E>()` driven by one name table per enum.
  - `CLI::parse` turns the text into enums once, and `Dither`, `Palette::nearest_index` and `temporal` take enums. No string is compared per pixel any more.
  - The error-diffusion algorithm is also checked in `CLI::parse` now, so a bad name is rejected before the image is loaded.
  - Algorithm, kernel, threshold matrix and palette stay strings, because each can also be a CSV path.
- **4.2 Static tables (§2.3 point 3):**
  - Every preset table lives in its own `.cpp` as a function-local `static const`, built once on first use: `src/grayscale.cpp`, `src/convolve.cpp`, `src/error_diffusion.cpp`, `src/palette.cpp` and `src/ordered.cpp`. The headers only declare the classes, plus `has_preset()` and `preset_names()`.
  - Threshold matrices use **option (b)**: Bayer matrices come from `Ordered::bayer(n)` with the recursive formula, and the 16 noise matrices are `std::uint8_t` arrays in the generated `src/ordered_tables.cpp`. Only the requested matrix is expanded.
  - `ErrorDiffusion` is now a list of `{dx, dy, scalar}` taps. `PairHash` and the coordinate/scalar maps are gone.
  - **Results:** `dither.o` is 48 KB (1.75 MB before) and `bin/dither` 0.8 MB (2.1 MB). Compiling `dither.cpp` takes 1.6 s (8.6 s). After touching `ordered.h`, a rebuild takes 3.7 s (11.7 s). cxxopts is now included only by `cli.cpp`, not by `main.cpp`. A serial clean build still takes about 29 s, because `cli.cpp` (cxxopts, 10 s) and lodepng dominate it.
- **4.3 Exceptions (§2.3 point 4):**
  - New `include/error.h` with `DitherError`. Everything below `main` now throws instead of calling `std::exit`: `Color`, `Palette`, `Convolve`, `Grayscale`, `Ordered`, `ErrorDiffusion`, `Image::load`/`save`, `Dither::temporal` and `CLI::parse`.
  - `main` catches the error, prints `Error: <message>` and returns 1. `-h` sets `CLI::help` instead of exiting.
  - `Image::load`/`save` and `Dither::load`/`save` now return `void` and throw.
  - Every bad input gives the same exit code as before. Messages are the same or clearer: CSV errors name the file and line.
  - New in-process test `library_errors_throw` (17 cases). The test runner now catches an exception escaping a test and reports it as a failure instead of aborting the run.
- **4.4 Shared CSV parser (§2.3 point 6):**
  - New `csv::read_matrix()` (`include/csv.h`, `src/csv.cpp`) is used for kernels, threshold matrices and grayscale weights.
  - It ignores blank lines, including trailing ones, and Windows line endings, and allows spaces and a leading `+`. A value is rejected unless the whole token parses as a number; before, `stod` accepted `"1.5abc"`.
  - Threshold values must be whole numbers from 0 to 255. Before, `"12.7"` was silently truncated to 12.
  - The grayscale CSV must be exactly one row of three values. Before, extra lines were ignored.
  - New test `csv_matrix_parser`.
- **4.5 Speed (Q5):**
  - The old `Color` distance functions, with their per-call default `std::vector`, are removed, along with `Color::add`, `equals`, `get_lightness`, `expand_range` and `grayscale_value`.
  - Grayscale weights are a `std::array<double, 3>` passed by `const&`. Every `Dither` method takes `const std::string&`.
  - `Color::CHANNEL_MAX` is now `int`, as 1.1 planned. `HEX_MAX` is `unsigned`.
  - `Image` getters are `const` and use `size_t` indices.
  - **Result:** the median whole run on `boat.png` is 4–11 ms faster: Floyd–Steinberg to `SLSO8` takes 67 ms (78 before) and `BLUE_NOISE_256X256` 68 ms (78), mostly because the tables are no longer rebuilt on every construction. The error-diffusion step on its own is about 16 ms, the same as Phase 3, because Phase 3 already removed the per-pixel copies.
- **4.6 Benchmark (Q11):**
  - `Benchmark` records named steps. `-b` now prints `load`, each operation in the order it ran, `save`, then `processing` (the operations only) and `total`.
  - The report is printed only after a successful save. With no operations, `processing` reads "no operations".
  - The accessors are `const`, and the tabs/spaces mix is gone.
  - The `-b` help text says what is timed.
  - New CLI test `benchmark_report`.
- **Also:**
  - `-Wsign-conversion` is now on, as 0.1 suggested; a clean build gives **0** first-party warnings.
  - First-party headers use `#pragma once` instead of reserved `__NAME_H` guards (REVIEW Q8).
  - `Image` no longer has the unused `Image(frames)`/`reset()`; `Dither()` no longer takes a frame count.
- **Tests:** 23 pass, 0 expected failures, under 1 s. There are 5 new tests (`outputs_unchanged`, `enum_names_round_trip`, `library_errors_throw`, `csv_matrix_parser`, `benchmark_report`), and `ed_kernel_sums` was rewritten for taps: each tap must point forward and be used once. Mutation-checked: changing one Floyd–Steinberg scalar, the blank-line handling or the benchmark report each makes the matching test fail.
- **Not done:**
  - Making a `Dither` method return `std::expected` instead of throwing; exceptions were simpler with no output change.
  - Moving the duplicated "-r/-e/-d/-t share one palette slot" logic; that is D2 in Phase 5.
  - Checking the MinGW build. Please run `make clean && make test` on Windows once.

---

## Phase 5 — Spec decisions and features → **v2.0.0** ✅

### 5.1 `UNIFORM_HISTOGRAM` *(S5)* — decision **D1: Option B**
- ~~**Option A (rename):** rename the current behaviour to `RANGE_STRETCH`, keep `UNIFORM_HISTOGRAM` as a deprecated alias, and correct the README.~~ Not chosen.
- **Option B (implement what the README says):**
  1. Build the image's luminance CDF.
  2. Sort the palette by lightness.
  3. Map each pixel's CDF value to palette rank `floor(cdf · N)`.
  4. Inside error diffusion or ordered dithering, apply the mapping to the adjusted value.

  This gives roughly equal amounts of each palette colour, as documented.

  **Done when:** on `boat.png` with `TWILIGHT5`, each colour covers 20 ± 5% of the image.

### 5.2 Per-operation palettes *(§2.3, point 2; S13)* — decision **D2: reject combinations**
- **Chosen.** The minimum change: if more than one of `-r`, `-e`, `-d` or `-t` is given, exit with an error saying only one quantising operation is allowed per run. Grayscale and convolve are pre-processing and can still be combined.
- ~~Alternatively, give each operation its own fields (`reduce_palette`, `ed_palette` and so on) and document the fixed order the operations run in.~~ Not chosen.

### 5.3 Deterministic PWM *(Q4)* — decision **D5: keep the stochastic version as `STOCHASTIC_PWM`**
- For each pixel, set `k = round(duty · frames)`. The pixel shows the lighter colour in frames `[(phase + i) mod frames for i < k]`, where `phase` comes from a blue-noise matrix lookup (one of the built-in ones). Keep the old behaviour as `-t STOCHASTIC_PWM`.
- Optionally choose the darker/lighter pair along the line through colour space between the two nearest palette colours, instead of by HSL lightness.
- **Done when:** for each pixel, the number of frames showing the lighter colour is `round(duty·frames)`, and the measured flicker drops on `tulips.png` with `PWM,8`.

### 5.4 Naming
- Accept `JARVIS_JUDICE_NINKE` and keep `JARVICE_JUDICE_NINKE` as an alias (S18).

### 5.5 Gamma correction for convolution — decision **D4: yes**
- **Files:** `src/dither.cpp`, `include/dither.h`, `src/main.cpp`, `src/cli.cpp`
- With `-a`, `Dither::convolve` converts the source to linear float, convolves there, and encodes back with one rounding. Without `-a` the output is byte-identical to Phase 4.
- **Done when:** averaging black and white with `-a` gives sRGB 188 (linear 50%), and 128 without `-a`.


### Phase 5 as built (30 Oct 2026)
**Output check:** 424 CLI runs on `boat.png` against the Phase 4 build (every error-diffusion kernel and scan pattern, six threshold matrices, reduce, with Euclidean and Manhattan mapping, four palettes including a CSV, every grayscale weighting, every convolution kernel, with and without `-a`). 420 are byte-identical. The other 4 combine `-c` with `-a`, which now convolves in linear light (D4, intended). In `outputs_unchanged`, only the `reduce UNIFORM_HISTOGRAM` hash changed, and it was updated on purpose.

- **5.1 `UNIFORM_HISTOGRAM` (D1 option B, S5, B4 spec part):**
  - It is now histogram equalisation onto the palette. The palette is sorted darkest to lightest. Each visible pixel gets a position `u = N · (pixels darker + half the pixels equal) / pixels` from the image's BT.709 luminance CDF (in working space, so `-a` uses linear light). Colour `k` covers `[k, k+1)`.
  - `reduce` picks `floor(u)`. Error diffusion quantises `u` the same way and diffuses the error in `u` (one unit = one palette step), using the same loop as colour error diffusion (a `diffuse<V>` template). Ordered dithering adds the centred threshold to `u` (pitch exactly 1).
  - An image with a single luminance has nothing to equalise, so it falls back to `EUCLIDEAN_DISTANCE` (keeps `uniform_hist_flat_channel`). Transparent pixels are left out of the histogram.
  - The range stretch is removed: `Palette::RangeMap`, `range_map()`, `UNMAPPED` and the min/max in the working buffer are gone. `Palette::nearest_index` treats `UNIFORM_HISTOGRAM` as Euclidean.
  - **Result on `boat.png` → `TWILIGHT5`:** every colour covers 19.5–20.5% for reduce, Floyd–Steinberg, Jarvis–Judice–Ninke, `BAYER_8X8` and `BLUE_NOISE_64X64`, with and without `-a` (target 20 ± 5%). Euclidean gives 44 / 21 / 16 / 12 / 7.5%.
  - Hue no longer affects the choice of colour (it is a luminance-rank mapping). The README says so.
- **5.2 One quantiser per run (D2, §2.3 point 2, S13):** `CLI::parse` rejects more than one of `-r`, `-e`, `-d` and `-t` with *"Only one of -r, -e, -d and -t can be used per run (-g and -c can be combined with any of them)"*, before the image is loaded. The README Usage section documents this and the fixed order.
- **5.3 Deterministic PWM (D5, Q4, S20):**
  - New `TemporalMethod::STOCHASTIC_PWM` keeps the v1 behaviour (a random draw per frame).
  - `PWM` now shows the lighter colour in exactly `k = round(duty · frames)` consecutive frames (wrapping), starting at a phase from `BLUE_NOISE_64X64`: `phase = floor((centred threshold + 0.5) · frames)`. Output is the same on every run.
  - The darker/lighter pair is still chosen by HSL lightness. The optional "line through colour space" idea was not done.
  - **Result on `tulips.png` → `SLSO8`, 8 frames:** local flicker (temporal standard deviation of 4×4 block luma) drops from 2.55 to 1.51, and the frame-to-frame standard deviation of the mean luma from 0.013 to 0.007. With `TWILIGHT5` and 4 frames: 2.80 → 1.86 and 0.032 → 0.008.
- **5.4 Naming (S18):** the preset is `JARVIS_JUDICE_NINKE`; `JARVICE_JUDICE_NINKE` is an alias. `has_preset` accepts both, `preset_names()` lists only the canonical name, and `ErrorDiffusion::name` holds the canonical name.
- **5.5 `-a` for convolution (D4, S11):** `Dither::convolve(kernel, gamma_correction)` samples linear float values with `-a` and encodes back once with `Transfer::to_encoded` (rounding, clamped). Without `-a` the output is byte-identical to Phase 4. The `-a` help text lists the operations it applies to.
- **Tests:** 5 new tests, each mutation-checked (disabling the feature makes it fail):
  - `uniform_hist_equal_shares` — a skewed (mostly dark) image gives 20 ± 3% per `TWILIGHT5` colour for reduce, raster and serpentine error diffusion, and ordered, with and without `-a`; the darkest/lightest pixel gets the darkest/lightest colour;
  - `cli_one_quantiser` — 5 combinations rejected with the message; 6 valid runs (including `-g -c -e` and `-a -c -d`) accepted;
  - `pwm_deterministic` — every pixel has exactly `round(duty · 8)` lighter frames, each frame is 25 ± 2% lighter, two runs are identical, and `STOCHASTIC_PWM` keeps the right mean but varies per pixel;
  - `jarvis_alias`;
  - `convolve_linear_light` — averaging black and white gives 188 with `-a` and 128 without.
  - `make test`: 28 pass, 0 expected failures, under 1 s. A clean build gives 0 first-party warnings.
- **Speed:** unchanged for the other mapping methods. `UNIFORM_HISTOGRAM` error diffusion on `boat.png` takes about 80 ms for the whole run.
- **README:** updated for the Phase 5 behaviour only (Usage, the `-a` row, `JARVIS_JUDICE_NINKE`, the temporal methods table, `UNIFORM_HISTOGRAM`). The rest of Phase 6 is still to do.
- **Not done:** checking the MinGW build. Please run `make clean && make test` on Windows once.

### 5.6 Release v2.0.0
- Tag the release. CHANGELOG: combining quantisers is an error; `UNIFORM_HISTOGRAM` is now histogram equalisation; `PWM` is deterministic and the old behaviour is `STOCHASTIC_PWM`; `-a` applies to `-c`; `JARVIS_JUDICE_NINKE` spelling.

---

## Phase 6 — Documentation *(S2–S20)*
These are README edits. They can be done at any time, but should match each release.

| Item | Edit |
|---|---|
| S2 | ✅ Error diffusion syntax → `-e ALGORITHM,SCAN_PATTERN,MAPPING_METHOD,PALETTE` |
| S3 | ✅ `--error-diffusion` → `--error_diffusion`, or add the hyphenated forms as aliases in cxxopts |
| S4 | ✅ Add `-o OUTPUT_FILE_PATH` to the temporal syntax line |
| S7, S8 | ✅ Document the BT.709-weighted Euclidean distance and describe Manhattan as L1. Consider an optional `UNWEIGHTED_EUCLIDEAN` |
| S10 | ✅ Correct the 8×8 Bayer claim (64 levels; 16×16 has all 256) |
| S11 | Define `-a` as "process in linear light", and list which operations use it. ✅ *The CLI table row was done in Phase 5; the Gamma Correction section in Phase 6.* |
| S13 | ✅ Document the fixed order operations run in, and the D2 outcome *(done in Phase 5)* |
| S14, S15 | ✅ Fix the "threshold matrixes" and "convolution kernels" copy-paste wording |
| S16 | ✅ Finish the "Why use Dither" list (temporal dithering, serpentine scanning, blue noise, Lospec CSVs) |
| S17 | ✅ State the minimum GCC version (13+; 0.1 has landed) |
| 4.4 | ✅ CSV formats: blank lines and Windows line endings are fine; threshold values must be whole numbers 0–255; grayscale weights are one row of three values |
| S18 | ✅ Use the `JARVIS_JUDICE_NINKE` spelling and mention the alias *(done in Phase 5)* |
| S5 | ✅ Describe `UNIFORM_HISTOGRAM` as histogram equalisation *(done in Phase 5)* |
| S20 | ✅ Describe PWM accurately, and `STOCHASTIC_PWM` *(done in Phase 5)* |
| Q11 | ✅ Describe the `-b` output: `load`, each operation, `save`, `processing` and `total`, printed after a successful save |
| — | ✅ Add a "Testing" section (`make test`, `XFAIL`/`TEST`, `-v`). *The CHANGELOG link is still to do: there is no CHANGELOG yet.* |

### Phase 6 as built (30 Oct 2026)
README edits only, in the existing style; no code changed. Every heading the tests parse is unchanged, and the preset names `preset_names_resolve` reads from the five tables are identical.
- **Syntax (S2, S3, S4):** the error diffusion line has `SCAN_PATTERN` and the correct `--error_diffusion` spelling, and links the scan pattern table; the temporal line has `-o OUTPUT_FILE_PATH` and states the 1–256 frame range.
- **Output formats (S12):** "input and output files must be .png" now reads "the input file must be .png; the output file may be .png or .gif", matching 1.4. The `-o` row says png or gif.
- **New sections:** *Gamma Correction* (S11: what linear light means, the black/white blur giving 188, the `gAMA`/`sRGB` chunk rule, which operations it applies to), *Benchmark* (Q11) and *Testing* (`make test`, `TEST`/`XFAIL`/`XPASS`, `-v`, running named tests). The `-a` and `-b` rows link to them.
- **Tables:** Euclidean is described as BT.709-weighted and Manhattan as unweighted L1 (S7, S8), with a note that `-a` measures distance in linear light; `RANDOM` states the 1/distance weights and the exact-match case (S19); the Temporal Dithering Methods table has an intro sentence (S14).
- **Wording:** "threshold matrixes" (S14, S15), the Bayer levels sentence (S10), GCC 13 or newer (S17), CSV rules for blank lines, CRLF, whole-number thresholds and one-row grayscale weights (4.4), and the "noticalbe" typo.
- **Why use Dither (S16):** blue noise matrixes and CSV threshold matrixes, temporal dithering, and histogram-equalised mapping. Serpentine scanning was left out, because it isn't clearly something ImageMagick lacks.
- **Not done:** the CHANGELOG link (no CHANGELOG exists yet), the optional `UNWEIGHTED_EUCLIDEAN` method, and cxxopts aliases for the hyphenated flags (the README now uses the real spelling instead). The README examples still have no reference images (3.5).

---

## Open decisions

| # | Decision | Recommendation |
|---|---|---|
| **D1** | `UNIFORM_HISTOGRAM`: rename to match the code, or implement true histogram mapping? | ✅ **Decided (30 Oct 2026): Option B.** Implement true histogram mapping in 5.1; the range stretch is replaced, not kept under another name. |
| **D2** | Allow combining quantising operations? | ✅ **Decided (30 Oct 2026): reject combinations.** More than one of `-r`, `-e`, `-d`, `-t` is an error; `-g` and `-c` can still be combined with any of them (5.2). |
| **D3** | Alpha handling | ✅ **Decided and implemented in Phase 3:** transparent pixels pass through unchanged, and alpha is kept on output. |
| **D4** | Should `-a` also apply to `-c`? | ✅ **Decided (30 Oct 2026): yes.** With `-a`, convolution runs in linear light (5.5). |
| **D5** | Keep the stochastic PWM? | ✅ **Decided (30 Oct 2026):** keep it as `STOCHASTIC_PWM`; the new deterministic version is the default `PWM` (5.3). |

---

## Suggested order of work and size

| Order | Task | Size |
|---|---|---|
| 1 | ~~0.1 Build fixes~~ ✅ | S |
| 2 | ~~0.2 Test harness and regression tests~~ ✅ | M |
| 3 | 1.1 Ordered overflow (B1) | XS |
| 4 | 1.2 Matrix key (B5) | XS |
| 5 | 1.3 UNIFORM_HISTOGRAM arithmetic (B4) | S |
| 6 | 1.4 Input validation (B6) | M |
| 7 | ~~2.1 Convolution (B2)~~ ✅ | M |
| 8 | ~~3.1–3.4 Float pipeline, gamma, threshold centring, alpha (B3, Q1, Q2, Q3, Q6)~~ ✅ | L |
| 9 | ~~4.1–4.6 Refactor~~ ✅ | L |
| 10 | ~~5.x Spec decisions~~ ✅ | M–L |
| — | 6 Docs | S, done alongside each release |

XS = under 30 min · S = about 1–2 h · M = half a day · L = 1–2 days
