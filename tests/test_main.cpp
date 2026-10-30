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

// Floyd-Steinberg to 2 colours in linear light. REVIEW.md B3 computes the correct
// white fraction for boat.png as 0.273; the current code gives 0.193.
XFAIL(gamma_linear_coverage, "B3: pixels linearised but palette left in sRGB")
{
    const fs::path boat = fs::path("input") / "boat.png";
    CHECK_MSG(fs::exists(boat), "input/boat.png is missing (run from the repository root)");

    if(!fs::exists(boat))
    {
        return;
    }

    Dither d = load(boat);
    d.error_diffusion("FLOYD_STEINBERG", "RASTER", "EUCLIDEAN_DISTANCE", "1BIT_MONITOR_GLOW", true);
    CHECK_NEAR(fraction_of(d, Color(0xf0f6f0)), 0.273, 0.02);
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
