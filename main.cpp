// Portable float & float16 pfm/phm image viewer
// Copyright Silicon Studio K.K. 2023
// author: Vivien Oddou
// BSD License

#include <iostream>
#include <ios>
#include <fstream>
#include <iterator>
#include <string_view>
#include <algorithm>

#include <simd_routines.h>  // auto generated by ispc and located in the build folder (config from cmake custom target)

#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <nana/gui/widgets/scroll.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/gui/msgbox.hpp>
#include <nana/paint/graphics.hpp>

#ifdef _WIN32
#define WINONLY(x) x
#include <io.h>
#include <fcntl.h>
#else
#define WINONLY(x)
#endif


namespace fs = std::filesystem;
using std::vector;
using std::string_view;
using namespace nana;

struct pfm_header
{
    std::string magic;
    int w = 0, h = 0;
    float scale_endian;

    char get_magic2ndchar() const { return magic.length() >= 2 ? magic[1] : 0; }
    bool is_half() const { return tolower(get_magic2ndchar()) == 'h'; }
    bool is_mono() const { return islower(get_magic2ndchar()); }
    int num_channels() const { return is_mono() ? 1 : 3; }
    size_t calc_raw_size() const { return w * h * num_channels() * sizeof(float) / (is_half() ? 2 : 1); }
};

bool pending_data(std::istream& is)
{
    is.seekg(0, is.end);
    auto length = is.tellg();
    is.seekg(0, is.beg);
    return !(length < 0);
}

msgbox message(std::string const& title, msgbox::icon_t ico, msgbox::button_t btn)
{
    msgbox m({}, title, btn);
    m.icon(ico);
    return m;
}

void data_bind(checkbox& cb, bool& data)
{
    cb.check(data);
    cb.events().click([&]() { data = cb.checked(); });
}

// using uint8 in ispc causes performance warnings so we use int8 on -128,127 range
// this function is the "signed to unsigned" remap
uint8_t stou(int8_t i)
{
    return uint8_t((int)i + 128);
}

nana::size min(nana::size const& a, nana::size const& b)
{
    return {std::min(a.width, b.width), std::min(a.height, b.height)};
}

struct app_state
{
    float exposure = 1.f;
    bool gamma = true;
    bool tone = true;
    bool flipy = false;
};

int main(int argc, char* argv[])
{
    fs::path inpath;
    std::ifstream infile;
    std::istream* in = nullptr;

    //Sleep(10000);

    if ((argc >= 2 && argv[1][0] == '-') || pending_data(std::cin))  // input is piped on stdin?
    {
        in = &std::cin;
        in->rdbuf()->pubsetbuf(nullptr, 0);  // deactivate buffering otherwise the stream explodes with failbit after 3450 bytes read
    }
    else if (argc <= 1)  // no command line -> open file dialog
    {
        filebox picker{nullptr, true};
        picker.title("Pick image file");
        picker.add_filter("Portable half map (.phm)", "*.phm");
        picker.add_filter("Portable float map (.pfm)", "*.pfm");
        vector<fs::path> paths = picker.show();
        if (!paths.empty())
            inpath = paths[0];
    }
    else if (argc >= 2)
        inpath = argv[1];

    if (!inpath.empty())
    {
        infile.open(inpath, std::ifstream::in | std::ifstream::binary);
        in = &infile;
    }

    if (!in || !*in) return 0; // no good input

    in->exceptions(std::ifstream::failbit | std::ifstream::badbit); // instant give up in case of issues

    pfm_header pfm;
    vector<char> raw;
    try
    {
        *in >> pfm.magic >> pfm.w >> pfm.h >> pfm.scale_endian;

        std::cout << "magic:" << pfm.magic << " w:" << pfm.w << " h:" << pfm.h << " scale_endian: " << pfm.scale_endian << "\n";

        size_t alloc = pfm.calc_raw_size();
        if (alloc == 0)
        {
            (message("No data", msgbox::icon_information, msgbox::ok) << "Width and height are 0 or not found")();
            return 2;
        }
        if (alloc > 1'000'000'000)
        {
            (message("Calculated image size too large", msgbox::icon_error, msgbox::ok)
             << "More than 1GiB of data needed because of parsed width:" << pfm.w << " and height:" << pfm.h)();
            return 3;
        }

        raw.resize(alloc);

        std::cout << "about to read " << alloc << " bytes\n";
        in->ignore(1);  // jump the last \n after scale_endian
        *in >> std::noskipws;
        in->clear();
        freopen(nullptr, "rb", stdin);  // go in binary mode from here
        WINONLY(_setmode(fileno(stdin), O_BINARY | O_RDONLY));
        // read bulk:
        in->sync();
        in->read(raw.data(), alloc);

        if (auto cnt = in->gcount(); cnt != alloc)
            std::cout << "not enough data read (" << cnt << " instead of " << alloc << " expected)\n";
        else if (std::cin.rdbuf()->in_avail() > 0)
            std::cout << "remaining data not read " << std::cin.rdbuf()->in_avail() << "\n";
        else 
            std::cout << "success\n";
    }
    catch (std::exception& e)
    {
        (message("Exception in input stream", msgbox::icon_error, msgbox::ok) << e.what())();
        return 4;
    }

    vector<int8_t> rgb;
    rgb.resize(pfm.w * pfm.h * pfm.num_channels());
    if (pfm.is_half())
        ispc::ToneAllF16PixelsAndToGamma((uint16_t*)raw.data(), rgb.data(), raw.size() / 2, 1.f);
    else
        ispc::ToneAllF32PixelsAndToGamma((float*)raw.data(), rgb.data(), raw.size() / 4, 1.f);

    paint::graphics surface(size(pfm.w, pfm.h));
    auto rgb_it = rgb.begin();
    if (pfm.num_channels() == 3)
        for (int y = 0; y < pfm.h; ++y)
            for (int x = 0; x < pfm.w; ++x)
            {
                surface.set_pixel(x, y, {stou(*rgb_it), stou(*(rgb_it + 1)), stou(*(rgb_it + 2))});
                rgb_it += 3;
            }
    else
        for (int y = 0; y < pfm.h; ++y)
            for (int x = 0; x < pfm.w; ++x)
            {
                uint8_t bw{stou(*rgb_it)};
                surface.set_pixel(x, y, {bw, bw, bw});
                ++rgb_it;
            }

    app_state state;

    // main window
    form mainwd{API::make_center(std::min(pfm.w, 1600) + 200, std::min(pfm.h, 1000))};
    mainwd.caption("Silicon Studio PFM/PHM viewer");
    group act{mainwd, "Options"};
    data_bind(act.add_option("Gamma"), state.gamma);
    data_bind(act.add_option("Filmic tone"), state.tone);
    data_bind(act.add_option("Flip Y"), state.flipy);
    act.radio_mode(false);
    slider exp{mainwd};
    exp.caption("exposure");
    exp.events().value_changed([&]() { exp.value(); });
    panel<false> panelZone{mainwd};
    picture pic{panelZone};
    nana::scroll<false> scrollH{panelZone};
    nana::scroll<true> scrollV{panelZone};
    auto resetAmounts = [&]()
    {
        scrollH.amount(std::max(0, pfm.w - (int)panelZone.size().width + 16));
        scrollV.amount(std::max(0, pfm.h - (int)panelZone.size().height + 16));
    };
    panelZone.events().resized(resetAmounts);
    place l2{panelZone};
    l2.div("<vert <<sub><scrollV weight=16>> <scrollH weight=16>>");
    l2["sub"] << pic;
    l2["scrollV"] << scrollV;
    l2["scrollH"] << scrollH;
    l2.collocate();
    drawing dr{pic};
    dr.draw([&](paint::graphics& a_g)
            {
                a_g.bitblt(rectangle({0,0}, min(panelZone.size(), a_g.size())), surface, {(int)scrollH.value(), (int)scrollV.value()});
            });
    scrollH.events().value_changed([&]() { API::refresh_window(pic); });
    scrollV.events().value_changed([&]() { API::refresh_window(pic); });
    place layout{mainwd};
    layout.div("<picdisplay>|200<vert controls arrange=[100,100,100]>");
    layout["picdisplay"] << panelZone;
    layout["controls"] << act << exp;
    layout.collocate();

    mainwd.show();
    exec();

    return 0;
}
