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
| 0.1 Build fixes | ✅ Done (30 Oct 2026) | Verified on GCC 13 / Linux. Not yet verified on MinGW. The new Makefile is in `Makefile.new` because the remote tools can't overwrite `Makefile`. |
| 0.2 Test harness | ✅ Done (30 Oct 2026) | `make test`: 1 pass, 8 expected failures, 0.3 s. |
| 1.1–1.4 Quick correctness wins | ✅ Done (30 Oct 2026) | `make test`: 7 pass, 2 expected failures (B2, B3). Verified on GCC 13 / Linux; not yet on MinGW. |
| 1.5 Release v1.0.1 | Not started | Rebuild `bin/dither.exe`, tag, CHANGELOG. |
| 2–6 | Not started | |

REVIEW.md §0.2 lists what Phase 0 changed in the findings; new finding Q12 (dead bounds checks) came out of it.

---

## Release overview

| Release | Phases | Theme | CLI compatible with 1.0? |
|---|---|---|---|
| **v1.0.1** | 0, 1 | Build fixes, tests, one-line and validation fixes | Yes |
| **v1.1.0** | 2, 3 | Correct convolution and a correct colour / gamma pipeline | Yes (output changes are bug fixes) |
| **v1.2.0** | 4 | Internal refactor: enums, static tables, error handling, speed | Yes |
| **v2.0.0** | 5, 6 | Spec decisions (histogram mapping, PWM, alpha, per-operation palettes) and a README rewrite | Possibly not; see the open decisions |

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

### 3.5 Release v1.1.0
- Generate the README examples again with the fixed binary into `docs/examples/`, and link them from the README so people can see the difference.

---

## Phase 4 — Internal refactor → **v1.2.0**
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
- Combine the raster and serpentine loops into one loop with `dir = ±1` and `x0/x1`. Use signed `int` coordinates so the bounds check is real (REVIEW Q12).
- Share one CSV matrix parser between `Convolve` and `Ordered`. While doing so, validate the 0–255 range for threshold matrices and ignore blank trailing lines.
- Share one `color_range()` helper (already done in 1.3).

### 4.5 Speed *(Q5)*
- Pass by `const&` throughout `Palette::nearest`.
- Replace the default `std::vector<double>` weights argument with a `static constexpr std::array`.
- Measure with `-b` before and after, using `boat.png` with Floyd–Steinberg to `SLSO8`.

### 4.6 Benchmark tidy-up *(Q11)*
- Print the time after `save()` and only if saving succeeded, or time load, processing and save separately.
- Optionally show the time for each operation.
- Make the accessors `const` and fix the stale include comment (done in 0.1).

---

## Phase 5 — Spec decisions and features → **v2.0.0**

### 5.1 `UNIFORM_HISTOGRAM` *(S5)* — decision **D1**
- **Option A (rename):** rename the current behaviour to `RANGE_STRETCH`, keep `UNIFORM_HISTOGRAM` as a deprecated alias, and correct the README.
- **Option B (implement what the README says):**
  1. Build the image's luminance CDF.
  2. Sort the palette by lightness.
  3. Map each pixel's CDF value to palette rank `floor(cdf · N)`.
  4. Inside error diffusion or ordered dithering, apply the mapping to the adjusted value.

  This gives roughly equal amounts of each palette colour, as documented.

  **Done when:** on `boat.png` with `TWILIGHT5`, each colour covers 20 ± 5% of the image.

### 5.2 Per-operation palettes *(§2.3, point 2; S13)* — decision **D2**
- The minimum change: if more than one of `-r`, `-e`, `-d` or `-t` is given, exit with an error saying only one quantising operation is allowed per run. Grayscale and convolve are pre-processing and can still be combined.
- Alternatively, give each operation its own fields (`reduce_palette`, `ed_palette` and so on) and document the fixed order the operations run in.

### 5.3 Deterministic PWM *(Q4)*
- For each pixel, set `k = round(duty · frames)`. The pixel shows the lighter colour in frames `[(phase + i) mod frames for i < k]`, where `phase` comes from a blue-noise matrix lookup (one of the built-in ones). Keep the old behaviour as `-t STOCHASTIC_PWM` if you want both.
- Optionally choose the darker/lighter pair along the line through colour space between the two nearest palette colours, instead of by HSL lightness.
- **Done when:** for each pixel, the number of frames showing the lighter colour is `round(duty·frames)`, and the measured flicker drops on `tulips.png` with `PWM,8`.

### 5.4 Naming
- Accept `JARVIS_JUDICE_NINKE` and keep `JARVICE_JUDICE_NINKE` as an alias (S18).

---

## Phase 6 — Documentation *(S2–S20)*
These are README edits. They can be done at any time, but should match each release.

| Item | Edit |
|---|---|
| S2 | Error diffusion syntax → `-e ALGORITHM,SCAN_PATTERN,MAPPING_METHOD,PALETTE` |
| S3 | `--error-diffusion` → `--error_diffusion`, or add the hyphenated forms as aliases in cxxopts |
| S4 | Add `-o OUTPUT_FILE_PATH` to the temporal syntax line |
| S7, S8 | Document the BT.709-weighted Euclidean distance and describe Manhattan as L1. Consider an optional `UNWEIGHTED_EUCLIDEAN` |
| S10 | Correct the 8×8 Bayer claim (64 levels; 16×16 has all 256) |
| S11 | Define `-a` as "process in linear light", and list which operations use it |
| S13 | Document the fixed order operations run in, and the D2 outcome |
| S14, S15 | Fix the "threshold matrixes" and "convolution kernels" copy-paste wording |
| S16 | Finish the "Why use Dither" list (temporal dithering, serpentine scanning, blue noise, Lospec CSVs) |
| S17 | State the minimum GCC version (13+; 0.1 has landed) |
| S20 | Describe PWM accurately (after 5.3) |
| Q11 | Say what `--benchmark` times |
| — | Add a "Testing" section (`make test`, `XFAIL`/`TEST`, `-v`) and a CHANGELOG link |

---

## Open decisions

| # | Decision | Recommendation |
|---|---|---|
| **D1** | `UNIFORM_HISTOGRAM`: rename to match the code, or implement true histogram mapping? | Do Option A in v1.0.1 (a doc fix), and Option B in v2.0.0. |
| **D2** | Allow combining quantising operations? | Reject combinations (one error message, simplest). Combining two quantisers rarely makes sense. |
| **D3** | Alpha handling | Pass transparent pixels through unchanged, and keep alpha on output. |
| **D4** | Should `-a` also apply to `-c`? | Yes, after Phase 3. Blurring in linear light looks more correct. |
| **D5** | Keep the stochastic PWM? | Keep it under a new name; make the deterministic version the default `PWM`. |

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
| 7 | 2.1 Convolution (B2) | M |
| 8 | 3.1–3.3 Float pipeline, gamma, threshold centring (B3, Q1, Q3, Q6) | L |
| 9 | 4.1–4.6 Refactor | L |
| 10 | 5.x Spec decisions | M–L |
| — | 6 Docs | S, done alongside each release |

XS = under 30 min · S = about 1–2 h · M = half a day · L = 1–2 days
