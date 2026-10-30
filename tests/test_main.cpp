// Dither regression tests (ROADMAP.md, Phase 0.2).
//
// Build and run from the repository root with:  make test
// Run by hand:                                  ./bin/test_dither ./bin/dither [-v] [test_name ...]
//
// The runner has no dependencies. Each test is a plain function that calls CHECK.
// Tests that are known to fail today are registered with XFAIL and the finding ID
// from REVIEW.md. When a fix lands, that test starts passing and the runner reports
// it as XPASS (and fails the run) until you change XFAIL to TEST below.
//
// The library tests call the Dither / Palette / Ordered APIs directly. The CLI tests
// run the real dither binary as a child process, so a crash or std::exit there
// can't take the runner down with it.

#include "convolve.h"
#include "dither.h"
#include "error_diffusion.h"
#include "grayscale.h"
#include "lodepng.h"
#include "ordered.h"
#include "palette.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Minimal test framework
// ---------------------------------------------------------------------------

namespace harness
{
    struct Failure
    {
        std::string where;
        std::string message;
    };

    struct Test
    {
        std::string name;
        std::function<void()> fn;
        bool expected_fail;
        std::string reason;
    };

    inline std::vector<Test>& registry()
    {
        static std::vector<Test> tests;
        return tests;
    }

    inline std::vector<Failure>& failures()
    {
        static std::vector<Failure> f;
        return f;
    }

    inline int& checks_run()
    {
        static int n = 0;
        return n;
    }

    inline void check(bool ok, const char* file, int line, const std::string& message)
    {
        checks_run()++;

        if(!ok)
        {
            failures().push_back({fs::path(file).filename().string() + ":" + std::to_string(line), message});
        }
    }

    struct Registrar
    {
        Registrar(std::string name, std::function<void()> fn, bool expected_fail, std::string reason)
        {
            registry().push_back({std::move(name), std::move(fn), expected_fail, std::move(reason)});
        }
    };
}

#define CHECK(expr) harness::check(static_cast<bool>(expr), __FILE__, __LINE__, #expr)
#define CHECK_MSG(expr, msg) harness::check(static_cast<bool>(expr), __FILE__, __LINE__, (msg))
#define CHECK_NEAR(actual, expected, tol) \
    do { \
        const double a_ = (actual), e_ = (expected), t_ = (tol); \
        std::ostringstream m_; \
        m_ << #actual << " = " << a_ << ", expected " << e_ << " +/- " << t_; \
        harness::check(std::fabs(a_ - e_) <= t_, __FILE__, __LINE__, m_.str()); \
    } while(0)

#define TEST_IMPL(name, xfail, reason) \
    static void name(); \
    static harness::Registrar registrar_##name(#name, name, xfail, reason); \
    static void name()

// TEST(name)            - must pass
// XFAIL(name, "B1 ...") - known to fail today; flip to TEST once the fix lands
#define TEST(name) TEST_IMPL(name, false, "")
#define XFAIL(name, reason) TEST_IMPL(name, true, reason)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace
{
    std::string g_dither_binary;  // set from argv[1]
    const fs::path TMP_DIR = fs::path("tests") / "tmp";

    struct RGBA
    {
        unsigned char r, g, b, a;
    };

    fs::path tmp_path(const std::string& name)
    {
        fs::create_directories(TMP_DIR);
        return TMP_DIR / name;
    }

    // Writes an RGBA8 PNG whose pixel (x, y) is given by fn(x, y).
    fs::path write_png(const std::string& name, unsigned w, unsigned h, const std::function<RGBA(unsigned, unsigned)>& fn)
    {
        std::vector<unsigned char> buf(static_cast<std::size_t>(w) * h * 4);

        for(unsigned y = 0; y < h; y++)
        {
            for(unsigned x = 0; x < w; x++)
            {
                const RGBA c = fn(x, y);
                const std::size_t i = (static_cast<std::size_t>(y) * w + x) * 4;
                buf[i + 0] = c.r;
                buf[i + 1] = c.g;
                buf[i + 2] = c.b;
                buf[i + 3] = c.a;
            }
        }

        const fs::path p = tmp_path(name);
        const unsigned err = lodepng::encode(p.string(), buf, w, h);

        if(err)
        {
            std::cerr << "write_png(" << p << "): " << lodepng_error_text(err) << std::endl;
            std::exit(2);
        }

        return p;
    }

    fs::path write_text(const std::string& name, const std::string& contents)
    {
        const fs::path p = tmp_path(name);
        std::ofstream(p, std::ios::binary) << contents;
        return p;
    }

    Dither load(const fs::path& p)
    {
        Dither d;

        if(d.load(p.string().c_str()))
        {
            std::cerr << "could not load " << p << std::endl;
            std::exit(2);
        }

        return d;
    }

    bool same_rgb(const Color& a, const Color& b)
    {
        return a.r == b.r && a.g == b.g && a.b == b.b;
    }

    // Fraction of pixels in frame 0 that equal `c` (RGB only).
    double fraction_of(Dither& d, const Color& c)
    {
        const std::size_t w = d.image.get_width();
        const std::size_t h = d.image.get_height();
        std::size_t n = 0;

        for(std::size_t y = 0; y < h; y++)
        {
            for(std::size_t x = 0; x < w; x++)
            {
                n += same_rgb(d.image.get_pixel(static_cast<unsigned>(x), static_cast<unsigned>(y)), c) ? 1 : 0;
            }
        }

        return static_cast<double>(n) / static_cast<double>(w * h);
    }

    std::string to_upper(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
        return s;
    }

    // --- CLI runner -------------------------------------------------------

    struct RunResult
    {
        bool crashed;      // killed by a signal / abnormal termination
        int exit_code;     // valid when !crashed
        std::string output; // stdout + stderr
    };

    std::string quote(const std::string& s)
    {
        return "\"" + s + "\"";
    }

    RunResult run_dither(const std::string& args)
    {
        const fs::path log = tmp_path("cli_output.txt");
        std::string bin = fs::path(g_dither_binary).make_preferred().string();
        std::string cmd = quote(bin) + " " + args + " > " + quote(log.string()) + " 2>&1";
#if defined(_WIN32)
        // cmd.exe strips the outermost pair of quotes, so wrap the whole command once more
        cmd = "\"" + cmd + "\"";
#endif
        const int status = std::system(cmd.c_str());

        RunResult r{false, 0, ""};
        {
            std::ifstream in(log, std::ios::binary);
            std::stringstream ss;
            ss << in.rdbuf();
            r.output = ss.str();
        }
#if defined(_WIN32)
        // NTSTATUS crash codes (0xC0000005 access violation, ...) and abort() (3)
        const unsigned code = static_cast<unsigned>(status);
        r.crashed = (code & 0xC0000000u) == 0xC0000000u || code == 3;
        r.exit_code = status;
#else
        if(WIFEXITED(status))
        {
            r.exit_code = WEXITSTATUS(status);
            // a shell reports a child killed by signal N as 128 + N
            r.crashed = r.exit_code > 128;
        }
        else
        {
            r.crashed = true;
            r.exit_code = -1;
        }
#endif
        return r;
    }

    bool printable(const std::string& s)
    {
        return std::all_of(s.begin(), s.end(), [](unsigned char c) { return c == '\n' || c == '\r' || c == '\t' || (c >= 0x20 && c < 0x7f); });
    }

    std::string first_line(const std::string& s)
    {
        const std::string line = s.substr(0, s.find('\n'));
        return line.size() > 100 ? line.substr(0, 100) + "..." : line;
    }

    // --- README parsing -----------------------------------------------------

    // Returns the names in the first column of the table under `heading`
    // (backticks optional), skipping the header row.
    std::vector<std::string> readme_table(const std::string& heading)
    {
        std::ifstream in("README.md");
        std::vector<std::string> names;
        std::string line;
        bool in_section = false;
        bool past_separator = false;

        while(std::getline(in, line))
        {
            if(!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            if(line.rfind("#", 0) == 0)
            {
                in_section = (line == heading);
                past_separator = false;
                continue;
            }

            if(!in_section || line.rfind("|", 0) != 0)
            {
                continue;
            }

            if(line.rfind("| :--", 0) == 0)
            {
                past_separator = true;
                continue;
            }

            if(past_separator)
            {
                std::string cell = line.substr(1, line.find('|', 1) - 1);
                std::erase(cell, '`');
                std::erase(cell, ' ');

                if(!cell.empty())
                {
                    names.push_back(cell);
                }
            }
        }

        return names;
    }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// Flat greys at 25%, 50% and 75% of the way from the dark to the light palette
// colour, dithered with BAYER_8X8, should come out 25%, 50% and 75% light.
// (50% alone can't catch B1: forcing every negative-offset cell to the darkest
// colour happens to be the right answer exactly at the midpoint.)
TEST(ordered_grey_coverage)
{
    const Color dark(0x222323), light(0xf0f6f0);

    for(const double level : {0.25, 0.50, 0.75})
    {
        auto lerp = [&](int a, int b) { return static_cast<unsigned char>(std::lround(a + level * (b - a))); };
        const RGBA grey{lerp(dark.r, light.r), lerp(dark.g, light.g), lerp(dark.b, light.b), 255};
        const fs::path p = write_png("grey_" + std::to_string(static_cast<int>(level * 100)) + ".png", 64, 64, [&](unsigned, unsigned) { return grey; });

        Dither d = load(p);
        d.ordered("BAYER_8X8", "EUCLIDEAN_DISTANCE", "1BIT_MONITOR_GLOW", false);
        const double got = fraction_of(d, light);
        CHECK_MSG(std::fabs(got - level) <= 0.02, std::to_string(static_cast<int>(level * 100)) + "% grey gave " + std::to_string(got) + " light, expected " + std::to_string(level) + " +/- 0.02");
    }
}

// An identity kernel must leave the image unchanged, edges included.
TEST(convolve_identity)
{
    auto pattern = [](unsigned x, unsigned y) {
        return RGBA{static_cast<unsigned char>(x * 13 + y * 7),
                    static_cast<unsigned char>(x * 3 + y * 29),
                    static_cast<unsigned char>((x ^ y) * 17), 255};
    };
    const unsigned W = 17, H = 11;
    const fs::path src = write_png("convolve_src.png", W, H, pattern);

    // rows x cols
    const std::vector<std::pair<int, int>> sizes = {{3, 3}, {5, 5}, {3, 5}};

    for(const auto& [rows, cols] : sizes)
    {
        std::string csv;

        for(int r = 0; r < rows; r++)
        {
            for(int c = 0; c < cols; c++)
            {
                csv += (r == rows / 2 && c == cols / 2) ? "1" : "0";
                csv += (c + 1 < cols) ? "," : "\n";
            }
        }

        const std::string label = std::to_string(rows) + "x" + std::to_string(cols);
        const fs::path kernel = write_text("identity_" + label + ".csv", csv);

        Dither d = load(src);
        d.convolve(kernel.string());

        int mismatches = 0;

        for(unsigned y = 0; y < H; y++)
        {
            for(unsigned x = 0; x < W; x++)
            {
                const RGBA e = pattern(x, y);
                mismatches += same_rgb(d.image.get_pixel(x, y), Color(e.r, e.g, e.b)) ? 0 : 1;
            }
        }

        CHECK_MSG(mismatches == 0, label + " identity kernel: " + std::to_string(mismatches) + " of " + std::to_string(W * H) + " pixels changed");
    }
}

// Blurring a solid image must give back the same solid colour, edges and corners
// included: with clamp-to-edge sampling there is no black border to bleed in.
TEST(convolve_blur_solid)
{
    const Color solid(100, 150, 200);
    const unsigned W = 9, H = 7;
    const fs::path src = write_png("convolve_solid.png", W, H, [](unsigned, unsigned) { return RGBA{100, 150, 200, 255}; });

    for(const std::string kernel : {"BOX_BLUR", "GAUSSIAN_BLUR"})
    {
        Dither d = load(src);
        d.convolve(kernel);
        int mismatches = 0;

        for(unsigned y = 0; y < H; y++)
        {
            for(unsigned x = 0; x < W; x++)
            {
                mismatches += same_rgb(d.image.get_pixel(x, y), solid) ? 0 : 1;
            }
        }

        CHECK_MSG(mismatches == 0, kernel + " on a solid image: " + std::to_string(mismatches) + " of " + std::to_string(W * H) + " pixels changed");
    }
}

// The kernel is flipped, so this is true convolution: out(p) = sum k(j) * in(p - j).
// A kernel with a single 1 in its top-left cell (j = (-1, -1)) must therefore move the
// image up and left by one pixel (a correlation would move it down and right), and the
// bottom row and right column repeat the edge.
TEST(convolve_is_true_convolution)
{
    auto pattern = [](unsigned x, unsigned y) {
        return RGBA{static_cast<unsigned char>(x * 20 + 10), static_cast<unsigned char>(y * 30 + 5), static_cast<unsigned char>((x * y) % 256), 255};
    };
    const unsigned W = 8, H = 6;
    const fs::path src = write_png("convolve_shift_src.png", W, H, pattern);
    const fs::path kernel = write_text("shift_3x3.csv", "1,0,0\n0,0,0\n0,0,0\n");

    Dither d = load(src);
    d.convolve(kernel.string());
    int mismatches = 0;

    for(unsigned y = 0; y < H; y++)
    {
        for(unsigned x = 0; x < W; x++)
        {
            const RGBA e = pattern(std::min(x + 1, W - 1), std::min(y + 1, H - 1));
            mismatches += same_rgb(d.image.get_pixel(x, y), Color(e.r, e.g, e.b)) ? 0 : 1;
        }
    }

    CHECK_MSG(mismatches == 0, "top-left impulse kernel: " + std::to_string(mismatches) + " of " + std::to_string(W * H) + " pixels differ from the image shifted up-left by 1");
}

// Convolution keeps the source alpha and rounds to nearest rather than truncating.
TEST(convolve_keeps_alpha_and_rounds)
{
    const unsigned W = 5, H = 5;
    const fs::path src = write_png("convolve_alpha.png", W, H, [](unsigned x, unsigned y) {
        return RGBA{10, 10, 10, static_cast<unsigned char>(x * 50 + y)};
    });
    // 0.97 * 10 = 9.7 -> rounds to 10 (truncation would give 9)
    const fs::path kernel = write_text("scale_097.csv", "0.97\n");

    Dither d = load(src);
    d.convolve(kernel.string());
    int alpha_changed = 0;
    int wrong_value = 0;

    for(unsigned y = 0; y < H; y++)
    {
        for(unsigned x = 0; x < W; x++)
        {
            const Color c = d.image.get_pixel(x, y);
            alpha_changed += c.a == static_cast<int>(x * 50 + y) ? 0 : 1;
            wrong_value += same_rgb(c, Color(10, 10, 10)) ? 0 : 1;
        }
    }

    CHECK_MSG(alpha_changed == 0, std::to_string(alpha_changed) + " pixels lost their alpha");
    CHECK_MSG(wrong_value == 0, std::to_string(wrong_value) + " pixels were not rounded to nearest (0.97 * 10 should give 10)");
}

// 2 colours in linear light. REVIEW.md B3 computes the correct white fraction for
// boat.png as 0.273 (the v1.0.0 code gave 0.193). Error diffusion and ordered
// dithering should both land there.
TEST(gamma_linear_coverage)
{
    const fs::path boat = fs::path("input") / "boat.png";
    CHECK_MSG(fs::exists(boat), "input/boat.png is missing (run from the repository root)");

    if(!fs::exists(boat))
    {
        return;
    }

    Dither ed = load(boat);
    ed.error_diffusion("FLOYD_STEINBERG", "RASTER", "EUCLIDEAN_DISTANCE", "1BIT_MONITOR_GLOW", true);
    CHECK_NEAR(fraction_of(ed, Color(0xf0f6f0)), 0.273, 0.02);

    Dither od = load(boat);
    od.ordered("BAYER_8X8", "EUCLIDEAN_DISTANCE", "1BIT_MONITOR_GLOW", true);
    CHECK_NEAR(fraction_of(od, Color(0xf0f6f0)), 0.273, 0.02);
}

// The sRGB curve and a pure gAMA power curve both round-trip every 8-bit level,
// are strictly increasing, and hit the standard reference values (ROADMAP 3.2).
TEST(transfer_round_trip)
{
    for(const double gamma : {0.0, 2.2, 1.8})
    {
        const Transfer t(gamma);
        int round_trip_failures = 0;
        int not_increasing = 0;

        for(int v = 0; v < 256; v++)
        {
            round_trip_failures += t.to_encoded(t.to_linear(v)) == v ? 0 : 1;
            not_increasing += (v > 0 && !(t.to_linear(v) > t.to_linear(v - 1))) ? 1 : 0;
        }

        const std::string label = gamma > 0.0 ? "gamma " + std::to_string(gamma) : "sRGB";
        CHECK_MSG(round_trip_failures == 0, label + ": " + std::to_string(round_trip_failures) + " of 256 levels do not round-trip");
        CHECK_MSG(not_increasing == 0, label + ": linear values are not strictly increasing");
        CHECK_NEAR(t.to_linear(0), 0.0, 1e-6);
        CHECK_NEAR(t.to_linear(255), 255.0, 1e-3);
    }

    // sRGB 128 is 21.586% linear; 10 is in the linear toe (10 / 255 / 12.92)
    const Transfer srgb(0.0);
    CHECK_NEAR(srgb.to_linear(128) / 255.0, 0.215861, 1e-5);
    CHECK_NEAR(srgb.to_linear(10) / 255.0, 10.0 / 255.0 / 12.92, 1e-7);
    CHECK_NEAR(Transfer(2.2).to_linear(128) / 255.0, std::pow(128.0 / 255.0, 2.2), 1e-6);
}

// PNG colour chunks: an sRGB chunk, or none, selects the sRGB curve (gamma 0);
// gAMA on its own selects a pure power curve with the file's exponent.
TEST(png_gamma_chunks)
{
    auto write_with = [](const std::string& name, bool gama, bool srgb) {
        lodepng::State state;
        state.info_png.gama_defined = gama ? 1 : 0;
        state.info_png.gama_gamma = 45455; // 1 / 2.2
        state.info_png.srgb_defined = srgb ? 1 : 0;
        state.info_png.srgb_intent = 0;
        state.encoder.auto_convert = 0;
        std::vector<unsigned char> rgba(4 * 4 * 4, 128), png;
        const fs::path p = tmp_path(name);
        lodepng::encode(png, rgba, 4, 4, state);
        lodepng::save_file(png, p.string());
        return p;
    };

    CHECK_NEAR(load(write_with("chunks_none.png", false, false)).image.get_gamma(), 0.0, 1e-9);
    CHECK_NEAR(load(write_with("chunks_gama.png", true, false)).image.get_gamma(), 2.2, 1e-3);
    CHECK_NEAR(load(write_with("chunks_both.png", true, true)).image.get_gamma(), 0.0, 1e-9);
}

// Grayscale with -a weights the channels in linear light and rounds once, so a
// grey ramp is unchanged and a colour gradient keeps (nearly) every grey level.
// (v1.0.0 kept 163 of them.)
TEST(grayscale_linear_levels)
{
    Dither ramp = load(write_png("grey_ramp.png", 256, 1, [](unsigned x, unsigned) {
        const unsigned char v = static_cast<unsigned char>(x);
        return RGBA{v, v, v, 255};
    }));
    ramp.grayscale("BT709", true);
    int changed = 0;

    for(unsigned x = 0; x < 256; x++)
    {
        const int v = static_cast<int>(x);
        changed += same_rgb(ramp.image.get_pixel(x, 0), Color(v, v, v)) ? 0 : 1;
    }

    CHECK_MSG(changed == 0, std::to_string(changed) + " of 256 grey ramp levels changed");

    Dither colour = load(write_png("colour_gradient.png", 256, 256, [](unsigned x, unsigned y) {
        return RGBA{static_cast<unsigned char>(x), static_cast<unsigned char>(y), static_cast<unsigned char>((x + y) / 2), 255};
    }));
    colour.grayscale("BT709", true);
    std::set<int> levels;

    for(unsigned y = 0; y < 256; y++)
    {
        for(unsigned x = 0; x < 256; x++)
        {
            levels.insert(colour.image.get_pixel(x, y).r);
        }
    }

    CHECK_MSG(levels.size() >= 250, "colour gradient kept " + std::to_string(levels.size()) + " grey levels, expected at least 250");
}

// Every built-in threshold matrix, not only BAYER_8X8, must reproduce flat greys at
// the right coverage once thresholds are centred (Q3). In v1.0.0 the small
// white-noise matrices were far off (WHITE_NOISE_2X2 gave 0% light at 25% grey),
// and averaged over greys from 5% to 95% BAYER_2X2 came out 40% light, not 50%,
// because "t - 127" shifted every threshold down by an eighth of a palette step.
TEST(ordered_all_matrices_coverage)
{
    const Color dark(0x222323), light(0xf0f6f0);
    std::vector<std::string> names;

    for(const auto& entry : Ordered("BAYER_2X2").THRESHOLD_MATRIXES)
    {
        names.push_back(entry.first);
    }

    std::sort(names.begin(), names.end());

    for(const double level : {0.25, 0.50, 0.75})
    {
        auto lerp = [&](int a, int b) { return static_cast<unsigned char>(std::lround(a + level * (b - a))); };
        const RGBA grey{lerp(dark.r, light.r), lerp(dark.g, light.g), lerp(dark.b, light.b), 255};
        const fs::path p = write_png("grey_all_" + std::to_string(static_cast<int>(level * 100)) + ".png", 256, 256, [&](unsigned, unsigned) { return grey; });

        for(const std::string& name : names)
        {
            Dither d = load(p);
            d.ordered(name, "EUCLIDEAN_DISTANCE", "1BIT_MONITOR_GLOW", false);
            const double got = fraction_of(d, light);
            CHECK_MSG(std::fabs(got - level) <= 0.02, name + ": " + std::to_string(static_cast<int>(level * 100)) + "% grey gave " + std::to_string(got) + " light");
        }
    }

    // unbiased: over greys at 5%, 15%, ..., 95% the average coverage is 50%
    std::vector<fs::path> ramp;

    for(int step = 0; step < 10; step++)
    {
        const double level = 0.05 + 0.1 * step;
        auto lerp = [&](int a, int b) { return static_cast<unsigned char>(std::lround(a + level * (b - a))); };
        const RGBA grey{lerp(dark.r, light.r), lerp(dark.g, light.g), lerp(dark.b, light.b), 255};
        ramp.push_back(write_png("grey_ramp_" + std::to_string(step) + ".png", 64, 64, [&](unsigned, unsigned) { return grey; }));
    }

    for(const std::string& name : names)
    {
        double total = 0.0;

        for(const fs::path& p : ramp)
        {
            Dither d = load(p);
            d.ordered(name, "EUCLIDEAN_DISTANCE", "1BIT_MONITOR_GLOW", false);
            total += fraction_of(d, light);
        }

        CHECK_MSG(std::fabs(total / 10.0 - 0.5) <= 0.02, name + ": greys 5-95% averaged " + std::to_string(total / 10.0) + " light, expected 0.5");
    }
}

// Error is carried in float, so even a 1-level error is diffused instead of being
// truncated away (Q1). In v1.0.0 a grey 1% of the way from dark to light came
// out 100% dark.
TEST(error_diffusion_keeps_small_error)
{
    const Color dark(0x222323), light(0xf0f6f0);

    for(const double level : {0.01, 0.99})
    {
        auto lerp = [&](int a, int b) { return static_cast<unsigned char>(std::lround(a + level * (b - a))); };
        const RGBA grey{lerp(dark.r, light.r), lerp(dark.g, light.g), lerp(dark.b, light.b), 255};
        // the exact level after rounding, measured with the same BT.709 weights as the distance
        const double actual = (0.2126 * (grey.r - dark.r) + 0.7152 * (grey.g - dark.g) + 0.0722 * (grey.b - dark.b)) /
                              (0.2126 * (light.r - dark.r) + 0.7152 * (light.g - dark.g) + 0.0722 * (light.b - dark.b));
        Dither d = load(write_png("grey_ed_" + std::to_string(static_cast<int>(level * 100)) + ".png", 128, 128, [&](unsigned, unsigned) { return grey; }));
        d.error_diffusion("FLOYD_STEINBERG", "RASTER", "EUCLIDEAN_DISTANCE", "1BIT_MONITOR_GLOW", false);
        CHECK_NEAR(fraction_of(d, light), actual, 0.003);
    }
}

// Alpha (D3): fully transparent pixels pass through unchanged and play no part in
// error diffusion; every other pixel keeps its alpha. Checked for every quantising
// operation.
TEST(alpha_preserved)
{
    const unsigned W = 32, H = 16;
    // left half transparent (its colour must not matter), right half a gradient with varying alpha
    auto image_with = [&](unsigned char hidden) {
        return [=](unsigned x, unsigned y) {
            if(x < W / 2)
            {
                return RGBA{hidden, static_cast<unsigned char>(255 - hidden), hidden, 0};
            }
            return RGBA{static_cast<unsigned char>(x * 8), static_cast<unsigned char>(y * 16), 128, static_cast<unsigned char>(1 + (x * 7 + y * 13) % 255)};
        };
    };
    const fs::path black = write_png("alpha_black.png", W, H, image_with(0));
    const fs::path white = write_png("alpha_white.png", W, H, image_with(255));

    const std::vector<std::pair<std::string, std::function<void(Dither&)>>> ops = {
        {"reduce", [](Dither& d) { d.reduce("EUCLIDEAN_DISTANCE", "SLSO8", false); }},
        {"error_diffusion", [](Dither& d) { d.error_diffusion("FLOYD_STEINBERG", "SERPENTINE", "EUCLIDEAN_DISTANCE", "SLSO8", true); }},
        {"ordered", [](Dither& d) { d.ordered("BAYER_4X4", "EUCLIDEAN_DISTANCE", "SLSO8", false); }},
        {"temporal", [](Dither& d) { d.temporal("RANDOM", 3, "EUCLIDEAN_DISTANCE", "SLSO8", false); }},
    };

    for(const auto& [label, op] : ops)
    {
        Dither b = load(black);
        Dither w = load(white);
        op(b);
        op(w);
        int transparent_changed = 0;
        int alpha_changed = 0;
        int influenced = 0;

        for(std::size_t frame = 0; frame < b.image.get_frames(); frame++)
        {
            for(unsigned y = 0; y < H; y++)
            {
                for(unsigned x = 0; x < W; x++)
                {
                    const RGBA e = image_with(0)(x, y);
                    const Color c = b.image.get_pixel(x, y, frame);

                    if(x < W / 2)
                    {
                        transparent_changed += (same_rgb(c, Color(e.r, e.g, e.b)) && c.a == 0) ? 0 : 1;
                    }
                    else
                    {
                        alpha_changed += c.a == e.a ? 0 : 1;
                        // what is hidden under the transparent half must not change the visible half
                        // (temporal is random per frame, so only the deterministic operations are compared)
                        if(label != "temporal")
                        {
                            const Color o = w.image.get_pixel(x, y, frame);
                            influenced += (same_rgb(c, o) && c.a == o.a) ? 0 : 1;
                        }
                    }
                }
            }
        }

        CHECK_MSG(transparent_changed == 0, label + ": " + std::to_string(transparent_changed) + " transparent pixels were changed");
        CHECK_MSG(alpha_changed == 0, label + ": " + std::to_string(alpha_changed) + " pixels lost their alpha");
        CHECK_MSG(influenced == 0, label + ": transparent pixels changed " + std::to_string(influenced) + " visible pixels");
    }
}

// [200, 100] and [100, 200] are the same data in a different order, so they must
// map to mirrored results, one dark pixel and one light pixel.
TEST(uniform_hist_order_independent)
{
    auto two = [](unsigned char a, unsigned char b) {
        return [=](unsigned x, unsigned) { const unsigned char v = x == 0 ? a : b; return RGBA{v, v, v, 255}; };
    };
    Dither up = load(write_png("uh_up.png", 2, 1, two(100, 200)));
    Dither down = load(write_png("uh_down.png", 2, 1, two(200, 100)));
    up.reduce("UNIFORM_HISTOGRAM", "1BIT_MONITOR_GLOW", false);
    down.reduce("UNIFORM_HISTOGRAM", "1BIT_MONITOR_GLOW", false);

    CHECK_MSG(!same_rgb(up.image.get_pixel(0, 0), up.image.get_pixel(1, 0)), "[100, 200] should map to two different colours, got " + up.image.get_pixel(0, 0).to_string_hex() + " " + up.image.get_pixel(1, 0).to_string_hex());
    CHECK_MSG(same_rgb(up.image.get_pixel(0, 0), down.image.get_pixel(1, 0)) && same_rgb(up.image.get_pixel(1, 0), down.image.get_pixel(0, 0)),
              "[200, 100] should mirror [100, 200], got " + down.image.get_pixel(0, 0).to_string_hex() + " " + down.image.get_pixel(1, 0).to_string_hex());
}

// A solid image has no range to stretch, so UNIFORM_HISTOGRAM should behave like
// plain nearest-colour mapping instead of dividing by zero.
TEST(uniform_hist_flat_channel)
{
    const fs::path p = write_png("solid.png", 4, 4, [](unsigned, unsigned) { return RGBA{100, 150, 200, 255}; });
    Dither uh = load(p);
    Dither eu = load(p);
    uh.reduce("UNIFORM_HISTOGRAM", "TWILIGHT5", false);
    eu.reduce("EUCLIDEAN_DISTANCE", "TWILIGHT5", false);
    CHECK_MSG(same_rgb(uh.image.get_pixel(0, 0), eu.image.get_pixel(0, 0)),
              "UNIFORM_HISTOGRAM gave " + uh.image.get_pixel(0, 0).to_string_hex() + ", EUCLIDEAN_DISTANCE gave " + eu.image.get_pixel(0, 0).to_string_hex());
}

// Every preset name in the README tables must exist in the matching built-in table.
TEST(preset_names_resolve)
{
    const Grayscale gray("STANDARD");
    const Convolve conv("BOX_BLUR");
    const ErrorDiffusion ed("LINEAR");
    const Ordered ord("BAYER_2X2");
    const Palette pal("TWILIGHT5");

    struct Section
    {
        std::string heading;
        std::function<bool(const std::string&)> exists;
    };

    const std::vector<Section> sections = {
        {"### Grayscale Channel Weights", [&](const std::string& n) { return gray.METHODS.contains(n); }},
        {"### Convolution Kernels", [&](const std::string& n) { return conv.KERNELS.contains(n); }},
        {"### Error Diffusion Algorithms", [&](const std::string& n) { return ed.COORDINATES.contains(n) && ed.SCALARS.contains(n); }},
        {"### Ordered Threshold Matrixes", [&](const std::string& n) { return ord.THRESHOLD_MATRIXES.contains(n); }},
        {"### Palettes", [&](const std::string& n) { return pal.PRESET_PALETTES.contains(n); }},
    };

    for(const auto& s : sections)
    {
        const auto names = readme_table(s.heading);
        CHECK_MSG(!names.empty(), "no names found in README section '" + s.heading + "'");

        for(const auto& n : names)
        {
            CHECK_MSG(s.exists(n), s.heading.substr(4) + ": " + n + " is documented but not built in");
        }
    }
}

// Every built-in threshold matrix should equal its CSV in data/threshold_matrixes/.
TEST(presets_match_csv)
{
    const Ordered presets("BAYER_2X2");
    const fs::path dir = fs::path("data") / "threshold_matrixes";
    std::set<std::string> seen;

    for(const auto& entry : fs::directory_iterator(dir))
    {
        if(entry.path().extension() != ".csv")
        {
            continue;
        }

        const std::string name = to_upper(entry.path().stem().string());
        seen.insert(name);

        if(!presets.THRESHOLD_MATRIXES.contains(name))
        {
            CHECK_MSG(false, name + ": CSV exists but there is no built-in preset with that name");
            continue;
        }

        const Ordered from_csv(entry.path().string());
        CHECK_MSG(from_csv.threshold_matrix == presets.THRESHOLD_MATRIXES.at(name), name + ": built-in values differ from " + entry.path().filename().string());
    }

    for(const auto& [name, _] : presets.THRESHOLD_MATRIXES)
    {
        CHECK_MSG(seen.contains(name), name + ": built-in preset has no CSV in data/threshold_matrixes/");
    }
}

// Each kernel must have a scalar for every offset, and the scalars must sum to 1
// (Atkinson deliberately diffuses only 3/4 of the error).
TEST(ed_kernel_sums)
{
    const ErrorDiffusion ed("LINEAR");

    for(const auto& [name, coords] : ed.COORDINATES)
    {
        CHECK_MSG(ed.SCALARS.contains(name), name + ": no scalars");

        if(!ed.SCALARS.contains(name))
        {
            continue;
        }

        const auto& scalars = ed.SCALARS.at(name);
        CHECK_MSG(scalars.size() == coords.size(), name + ": coordinate count != scalar count");
        double sum = 0.0;

        for(const auto& c : coords)
        {
            CHECK_MSG(scalars.contains(c), name + ": offset (" + std::to_string(c.first) + "," + std::to_string(c.second) + ") has no scalar");
            sum += scalars.contains(c) ? scalars.at(c) : 0.0;
        }

        const double expected = (name == "ATKINSON") ? 0.75 : 1.0;
        CHECK_MSG(std::fabs(sum - expected) < 1e-9, name + ": scalars sum to " + std::to_string(sum) + ", expected " + std::to_string(expected));
    }
}

// The B6 inputs from REVIEW.md: each must either be rejected (non-zero exit, a
// readable message, no crash) or, where the input is legitimate, succeed correctly.
TEST(cli_rejects_bad_input)
{
    if(g_dither_binary.empty() || !fs::exists(g_dither_binary))
    {
        CHECK_MSG(false, "dither binary not found: '" + g_dither_binary + "' (pass it as the first argument)");
        return;
    }

    const std::string in = quote(write_png("cli_in.png", 8, 8, [](unsigned x, unsigned y) {
        const unsigned char v = static_cast<unsigned char>((x + y) * 16); return RGBA{v, v, v, 255};
    }).string());
    const std::string out_png = quote(tmp_path("cli_out.png").string());
    const std::string out_gif = quote(tmp_path("cli_out.gif").string());
    const std::string pal_empty = quote(write_text("pal_empty.csv", "").string());
    const std::string pal_none = quote(write_text("pal_no_colours.csv", "nocolours,tester\n").string());
    const std::string pal_one = quote(write_text("pal_one.csv", "one,tester,808080\n").string());
    const std::string tm_range = quote(write_text("tm_out_of_range.csv", "0,300\n128,64\n").string());
    const std::string missing_dir_gif = quote((TMP_DIR / "no_such_dir" / "out.gif").string());

    struct Case
    {
        std::string label;
        std::string args;
        bool should_succeed; // false: must be rejected
    };

    const std::vector<Case> cases = {
        {"-r with one argument", "-i " + in + " -o " + out_png + " -r TWILIGHT5", false},
        {"-t non-numeric frame count", "-i " + in + " -o " + out_gif + " -t PWM,abc,EUCLIDEAN_DISTANCE,TWILIGHT5", false},
        {"-t negative frame count", "-i " + in + " -o " + out_gif + " -t PWM,-1,EUCLIDEAN_DISTANCE,TWILIGHT5", false},
        {"-t zero frames", "-i " + in + " -o " + out_gif + " -t PWM,0,EUCLIDEAN_DISTANCE,TWILIGHT5", false},
        {"-o unknown extension", "-i " + in + " -o " + quote(tmp_path("cli_out.jpg").string()) + " -r EUCLIDEAN_DISTANCE,TWILIGHT5", false},
        {"palette CSV with no colours", "-i " + in + " -o " + out_png + " -r EUCLIDEAN_DISTANCE," + pal_none, false},
        {"palette CSV empty", "-i " + in + " -o " + out_png + " -r EUCLIDEAN_DISTANCE," + pal_empty, false},
        {"-t with UNIFORM_HISTOGRAM", "-i " + in + " -o " + out_gif + " -t PWM,4,UNIFORM_HISTOGRAM,TWILIGHT5", false},
        {"-t with unknown mapping method", "-i " + in + " -o " + out_gif + " -t RANDOM,4,FOO,TWILIGHT5", false},
        {"-e with unknown scan pattern", "-i " + in + " -o " + out_png + " -e FLOYD_STEINBERG,SNAKE,EUCLIDEAN_DISTANCE,TWILIGHT5", false},
        {"threshold CSV value above 255", "-i " + in + " -o " + out_png + " -d " + tm_range + ",EUCLIDEAN_DISTANCE,TWILIGHT5", false},
        {"GIF into a missing folder", "-i " + in + " -o " + missing_dir_gif + " -t PWM,4,EUCLIDEAN_DISTANCE,TWILIGHT5", false},
        {"1-colour palette with -d", "-i " + in + " -o " + out_png + " -d BAYER_4X4,EUCLIDEAN_DISTANCE," + pal_one, true},
    };

    for(const auto& c : cases)
    {
        std::error_code ec;
        fs::remove(tmp_path("cli_out.png"), ec);

        const RunResult r = run_dither(c.args);
        const std::string got = r.crashed ? "crashed" : "exit " + std::to_string(r.exit_code);

        CHECK_MSG(!r.crashed, c.label + ": crashed (status " + std::to_string(r.exit_code) + ")");
        CHECK_MSG(printable(r.output), c.label + ": output contains unprintable bytes (garbage memory?)");

        if(c.should_succeed)
        {
            CHECK_MSG(!r.crashed && r.exit_code == 0, c.label + ": expected success, got " + got + " - " + first_line(r.output));

            if(!r.crashed && r.exit_code == 0)
            {
                Dither d = load(tmp_path("cli_out.png"));
                CHECK_MSG(fraction_of(d, Color(0x808080)) == 1.0, c.label + ": every pixel should be #808080");
            }
        }
        else if(!r.crashed)
        {
            CHECK_MSG(r.exit_code != 0, c.label + ": expected a non-zero exit, got exit 0");
            CHECK_MSG(r.output.find("rror") != std::string::npos, c.label + ": no error message printed");
        }
    }
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    bool verbose = false;
    std::set<std::string> only;

    for(int i = 1; i < argc; i++)
    {
        const std::string a = argv[i];

        if(a == "-v" || a == "--verbose")
        {
            verbose = true;
        }
        else if(g_dither_binary.empty() && (a.find('/') != std::string::npos || a.find('\\') != std::string::npos))
        {
            g_dither_binary = a;
        }
        else
        {
            only.insert(a);
        }
    }

    if(!fs::exists("README.md") || !fs::exists("include"))
    {
        std::cerr << "run the tests from the repository root (make test)" << std::endl;
        return 2;
    }

    const auto t0 = std::chrono::steady_clock::now();
    int pass = 0, fail = 0, xfail = 0, xpass = 0;

    for(const auto& t : harness::registry())
    {
        if(!only.empty() && !only.contains(t.name))
        {
            continue;
        }

        harness::failures().clear();
        harness::checks_run() = 0;
        t.fn();
        const auto& f = harness::failures();
        const bool ok = f.empty();
        const char* tag;

        if(!t.expected_fail)
        {
            tag = ok ? "PASS " : "FAIL ";
            (ok ? pass : fail)++;
        }
        else
        {
            tag = ok ? "XPASS" : "xfail";
            (ok ? xpass : xfail)++;
        }

        std::cout << "[" << tag << "] " << t.name;

        if(t.expected_fail && !ok)
        {
            std::cout << "  (" << f.size() << "/" << harness::checks_run() << " checks failing; " << t.reason << ")";
        }

        std::cout << std::endl;

        if(t.expected_fail && ok)
        {
            std::cout << "        now passes - change XFAIL to TEST for " << t.name << " in tests/test_main.cpp" << std::endl;
        }

        // always show why a must-pass test failed; show expected failures with -v
        if(!ok && (!t.expected_fail || verbose))
        {
            for(const auto& fl : f)
            {
                std::cout << "        " << fl.where << ": " << fl.message << std::endl;
            }
        }
    }

    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::cout << "\n" << pass << " passed, " << fail << " failed, " << xfail << " expected failures, " << xpass << " unexpected passes"
              << " (" << std::fixed << std::setprecision(2) << secs << "s)" << std::endl;

    if(xfail > 0 && !verbose)
    {
        std::cout << "run with -v to see the details of expected failures:  " << argv[0] << " " << g_dither_binary << " -v" << std::endl;
    }

    return (fail == 0 && xpass == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
