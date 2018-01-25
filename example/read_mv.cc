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

#include "../src/decoder/h264.hh"
#include "../src/util/argparser.hh"
#include "../src/util/exception.hh"

using namespace std;

int main(int argc, char * argv[]) {
    ArgParser parser("Read motion vectors from a media file "
                     "and print them out");
    parser.add_arg("-i", "input", "media file input");
    parser.add_arg("-n", "frame_num", "frame number to extract");
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();
    string filename = arg_values["input"];
    uint32_t frame_num = (uint32_t)stoi(arg_values["frame_num"]);

    unique_ptr<h264> decoder = make_unique<h264>(filename);

    try {
        uint64_t index_size = decoder->index_size();
        if (frame_num >= index_size) {
            cerr << "frame: " << frame_num << " outside of range ("
                 << index_size << ")" << endl;
            return EXIT_FAILURE;
        }
        MvFrame frame;
        bool p_slice;
        tie(frame, p_slice) = decoder->load_frame(frame_num);
        cout << "Frame size: " << frame.width() << "x" << frame.height()
             << "P-slice ? " << p_slice << endl;
        uint32_t counter = 0;
        for (uint32_t y = 0; y < frame.mb_height(); y++) {
            for (uint32_t x = 0; x < frame.mb_width(); x++) {
                auto mv = frame.get_mv(x, y);
                cout << "x: " << setfill('0') << setw(2) << x << " y: "
                     << setfill('0') << setw(2) << y << " mvL0: ("
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
