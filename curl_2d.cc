#include "clumpy_command.hh"
#include "fmt/core.h"
#include "cnpy/cnpy.h"

using namespace std;

struct Curl2d : ClumpyCommand {
    Curl2d() {}
    bool exec(vector<string> args) override;
    string description() const override {
        return "apply the curl operator to a field of scalars";
    }
    string usage() const override {
        return "<input_img> <output_img>";
    }
    string example() const override {
        return "in.npy out.npy";
    }
};

static ClumpyCommand::Register registrar("curl_2d", [] {
    return new Curl2d();
});

bool Curl2d::exec(vector<string> vargs) {
    if (vargs.size() != 2) {
        fmt::print("Wrong number of arguments.\n");
        return false;
    }
    const string input_file = vargs[0];
    const string output_file = vargs[1];

    cnpy::NpyArray arr = cnpy::npy_load(input_file);
    if (arr.shape.size() != 2) {
        fmt::print("Input data has wrong shape.\n");
        return false;
    }
    if (arr.word_size != sizeof(float)) {
        fmt::print("Input data has wrong word size.\n");
        return false;
    }
    uint32_t height = arr.shape[0];
    uint32_t width = arr.shape[1];

    float const* psrc = arr.data<float>();
    vector<float> result(width * height * 2);
    float* pdstx = result.data();
    float* pdsty = result.data() + width * height;

    float minval = numeric_limits<float>::max();
    float maxval = numeric_limits<float>::lowest();

    for (uint32_t row = 0; row < height; ++row) {
        for (uint32_t col = 0; col < width; ++col) {
            int nextcol = (col < width - 1) ? (col + 1) : col;
            int nextrow = (row < height - 1) ? (row + 1) : row;
            float p = psrc[col + row * width];
            float dpdx = psrc[nextcol + row * width] - p;
            float dpdy = psrc[col + nextrow * width] - p;
            *pdstx++ = dpdy;
            *pdsty++ = -dpdx;
            minval = std::min(minval, *(pdstx - 1));
            maxval = std::max(maxval, *(pdstx - 1));
            minval = std::min(minval, *(pdsty - 1));
            maxval = std::max(maxval, *(pdsty - 1));
        }
    }

    fmt::print("Curl range is {} to {}\n", minval, maxval);
    fmt::print("Curl shape is 2x{}x{}\n", width, height);
    cnpy::npy_save(output_file, result.data(), {2, height, width}, "w");
    return true;
}
