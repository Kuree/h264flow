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

#include "../src/h264.hh"
#include <experimental/filesystem>

using namespace std;
namespace fs = std::experimental::filesystem;

bool is_mp4(char * filename) {
    fs::path path(filename);
    return path.extension().string() == ".mp4";
}

bool is_raw(char * filename) {
    fs::path path(filename);
    return path.extension().string() == ".264"
           || path.extension().string() == ".h264";
}

int main(int argc, char * argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <file_name> <frame_number>" << endl;
        return EXIT_FAILURE;
    }
    char * filename = argv[1];
    uint32_t frame_num = (uint32_t)stoi(argv[2]);
    unique_ptr<h264> decoder = nullptr;

    if (is_mp4(filename)) {
        shared_ptr<MP4File> mp4 = make_shared<MP4File>(filename);
        cerr << "WARN: MPEG-4/AVC is untested" << endl;
        decoder = make_unique<h264>(mp4);
    } else if(is_raw(filename)) {
        auto bs = make_shared<BitStream>(filename);
        decoder = make_unique<h264>(bs);
    } else {
        cerr << "Unsupported file format" << endl;
        return EXIT_FAILURE;
    }
    try {
        std::shared_ptr<MvFrame> frame = decoder->load_frame(frame_num);
        if (!frame)
            throw std::runtime_error("Not a P-slice");
        cout << "Frame size: " << frame->width() << "x" << frame->height()
             << endl;
        uint32_t counter = 0;
        for (uint32_t y = 0; y < frame->mb_height(); y++) {
            for (uint32_t x = 0; x < frame->mb_width(); x++) {
                auto mv = frame->get_mv(x, y);
                cout << "x: " << x << " y: " << y << " mvL0: ("
                     <<  mv.mvL0[0] << ", " << mv.mvL0[1] << ")";
                if (counter++ % 3 == 2)
                    cout << endl;
                else
                    cout << "    ";
            }
        }
    } catch (const NotImplemented & ex) {
        cerr << "ERROR " << ex.what() << endl;
    }
}
