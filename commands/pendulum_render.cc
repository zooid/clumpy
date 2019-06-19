#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

#include <glm/vec3.hpp>
#include <glm/ext.hpp>

#include <blend2d.h>

#include <limits>

using namespace glm;

using std::vector;
using std::string;
using std::numeric_limits;

namespace {

struct PendulumRender : ClumpyCommand {
    PendulumRender() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "";
    }
    string usage() const override {
        return "<image_type> <dims> <friction> <scale_x> <scale_y> <output_img>";
    }
    string example() const override {
        return "500x500 0.01 1.0 5.0 field.npy";
    }
    void drawLeftPanel(BLImage* dst);
    void drawRightPanel(BLImage* dst);
};

static ClumpyCommand::Register registrar("pendulum_render", [] {
    return new PendulumRender();
});

void PendulumRender::drawLeftPanel(BLImage* img) {
    BLContext ctx(*img);

    // https://blend2d.com/doc/getting-started.html
    // if blend2d doesn't work out...
    //     https://github.com/prideout/skia
    //     https://skia.org/user/api/skcanvas_creation

    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.fillAll();

    ctx.setCompOp(BL_COMP_OP_SRC_OVER);

    double cx = 180, cy = 180, r = 30;

    ctx.setStrokeWidth(10);
    ctx.setStrokeStartCap(BL_STROKE_CAP_ROUND);
    ctx.setStrokeEndCap(BL_STROKE_CAP_BUTT);
    ctx.setStrokeStyle(BLRgba32(0xff7f7f7f));

    BLPath path;
    path.moveTo(img->width() / 2, img->height() / 2);
    path.lineTo(cx, cy);
    ctx.strokePath(path);

    BLGradient radial(BLRadialGradientValues(cx + r/3, cy + r/3, cx + r/3, cy + r/3, r * 1.75));
    radial.addStop(0.0, BLRgba32(0xff5cabff));
    radial.addStop(1.0, BLRgba32(0xff000000));
    ctx.setFillStyle(radial);
    ctx.fillCircle(cx, cy, r);
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF));

    ctx.end();
}

void PendulumRender::drawRightPanel(BLImage* img) {
    BLContext ctx(*img);
    ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    ctx.setFillStyle(BLRgba32(0));
    ctx.fillAll();

    ctx.setCompOp(BL_COMP_OP_SRC_OVER);

    ctx.setFillStyle(BLRgba32(0xffffffff));
    BLFontFace face;
    BLResult err = face.createFromFile("extras/NotoSans-Regular.ttf");
    if (err) {
        fmt::print("Failed to load a font-face: {}\n", err);
        return;
    }

    BLFont font;
    font.createFromFace(face, 20.0f);

    BLFontMetrics fm = font.metrics();
    BLTextMetrics tm;
    BLGlyphBuffer gb;

    BLPoint p(20, 190 + fm.ascent);
    const char* text = "Hello Blend2D!\n"
                        "I'm a simple multiline text example\n"
                        "that uses BLGlyphBuffer and fillGlyphRun!";
    for (;;) {
        const char* end = strchr(text, '\n');
        gb.setUtf8Text(text, end ? (size_t)(end - text) : SIZE_MAX);
        font.shape(gb);
        font.getTextMetrics(gb, tm);

        p.x = (480.0 - (tm.boundingBox.x1 - tm.boundingBox.x0)) / 2.0;
        ctx.fillGlyphRun(p, font, gb.glyphRun());
        p.y += fm.ascent + fm.descent + fm.lineGap;

        if (!end) break;
        text = end + 1;
    }

    ctx.end();
}

bool PendulumRender::exec(vector<string> vargs) {
    if (vargs.size() != 5) {
        fmt::print("The command takes 5 arguments.\n");
        return false;
    }
    const string dims = vargs[0];
    const float friction = atof(vargs[1].c_str());
    const float scalex = atof(vargs[2].c_str());
    const float scaley = atof(vargs[3].c_str());
    const string output_file = vargs[4];
    const uint32_t output_width = atoi(dims.c_str());
    const uint32_t output_height = atoi(dims.substr(dims.find('x') + 1).c_str());

    BLImage limg(output_width, output_height, BL_FORMAT_PRGB32);
    drawLeftPanel(&limg);

    BLImage rimg(output_width, output_height, BL_FORMAT_PRGB32);
    drawRightPanel(&rimg);

    BLImageData limgdata;
    limg.getData(&limgdata);

    BLImageData rimgdata;
    rimg.getData(&rimgdata);

    const u8vec4* lsrcpixels = (const u8vec4*) limgdata.pixelData;
    const u8vec4* rsrcpixels = (const u8vec4*) rimgdata.pixelData;

    vector<vec4> dstimg(output_width * 2 * output_height);
    for (uint32_t row = 0; row < output_height; ++row) {
        for (uint32_t col = 0; col < output_width; ++col) {
            u8vec4 src = u8vec4(lsrcpixels[row * output_width + col]);
            dstimg[row * output_width * 2 + col] = vec4(src) / 255.0f;
        }
        for (uint32_t col = 0; col < output_width; ++col) {
            u8vec4 src = u8vec4(rsrcpixels[row * output_width + col]);
            dstimg[row * output_width * 2 + col + output_width] = vec4(src) / 255.0f;
        }
    }
    cnpy::npy_save(output_file, &dstimg.data()->x, {output_height, output_width * 2, 4}, "w");
    return true;
}

} // anonymous namespace
