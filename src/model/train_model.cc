/*  This file is part of h264flow.

    h264flow is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    h264flow is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with h264flow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "tiny-dnn/tiny_dnn/tiny_dnn.h"
#include <dirent.h>
#include "../decoder/util.hh"
#include "model-io.hh"


#define LABEL_SIZE 7

using namespace std;
using namespace tiny_dnn;
using namespace tiny_dnn::layers;

using relu     = tiny_dnn::activation::relu;
using conv     = tiny_dnn::layers::conv;
using max_pool = tiny_dnn::layers::max_pool;
using softmax  = tiny_dnn::activation::softmax;

vector<string> get_file_listing(const string &dir) {
    vector<std::string> files;
    shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir) {
        if(dir) closedir(dir);
    });
    struct dirent *dir_ptr;
    if (!directory_ptr) {
        cerr << "Error opening : " << strerror(errno) << dir << endl;
        return files;
    }

    while ((dir_ptr = readdir(directory_ptr.get())) != nullptr) {
        files.emplace_back(std::string(dir_ptr->d_name));
    }
    return files;
}

vector<vec_t> get_training_data(const vector<MvFrame> & mvs) {
    const uint32_t height = mvs[0].mb_height();
    const uint32_t width = mvs[0].mb_width();
    vector<vec_t> result(mvs.size());
    uint32_t count = 0;
    for (const auto & mv: mvs) {
        if (mv.mb_height() != height || mv.mb_width() != width)
            throw runtime_error("data set dimension does not match.");
        vec_t entry(mv.mb_height() * mv.mb_width() * 2);
        for (uint32_t i = 0; i < height; i ++) {
            for (uint32_t j = 0; j < width; j++) {
                auto v = mv.get_mv(j, i);
                entry[(i * width + j) * 2] = v.mvL0[0];
                entry[(i * width + j) * 2 + 1] = v.mvL0[1];
            }
        }
        result[count++] = entry;
    }
    return result;
}

vec_t create_label(const uint32_t label) {
    /* this is dependent on the annotator */
    vec_t result(LABEL_SIZE);
    if (!label)
        result[0] = 1;
    for (int i = 0; i < LABEL_SIZE - 1; i++) {
        if (label & (1 << i))
            result[i + 1] = 1;
    }
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <data_dir>" << endl;
        return EXIT_FAILURE;
    }

    string data_dir = argv[1];
    vector<MvFrame> mv_frames;
    vector<MvFrame> test_frames;
    vector<vec_t> labels;
    vector<vec_t> test_labels;

    uint32_t num_of_tests = 5;

    /* get all data files */
    auto files = get_file_listing(data_dir);
    uint32_t counter = 0;
    for (auto const & f : files) {
        if (file_extension(f) == ".mv") {
            /* TODO: fix this */
            string filename = data_dir + "/" +f;
            MvFrame mv; uint32_t label;
            tie(mv, label) = load_mv(filename);
            /* TODO: fix this. need more randomness */
            if (counter < files.size() - num_of_tests) {
                mv_frames.emplace_back(mv);
                labels.emplace_back(create_label(label));
            } else {
                test_frames.emplace_back(mv);
                test_labels.emplace_back(create_label(label));
            }
            counter++;
        }
    }

    vector<vec_t> inputs = get_training_data(mv_frames);
    vector<vec_t> test_inputs = get_training_data(test_frames);


    /* create a simple DNN */
    uint32_t width = mv_frames[0].mb_width();
    uint32_t height = mv_frames[0].mb_height();
    network<sequential> net;
    net << conv(width, height, 5, 2, 2, padding::same) << relu()
        << ave_pool(width, height, 2, 5) << relu()
        << fc( width / 5 * height / 5 * 2, 120) << tiny_dnn::activation::tanh()
        << fc(120, LABEL_SIZE) << softmax();


    adagrad opt;
    size_t batch_size = 1;
    int epochs = 30;

    net.fit<mse>(opt, inputs, labels, batch_size, epochs);

    return EXIT_SUCCESS;
}
