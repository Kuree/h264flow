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
#include "../src/query/operator.hh"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 8) {
        cerr << "Usage: " << argv[0] << "<filename> <frame_num> <rect_x> "
                "<rect_y> <rect_width> <rect_height> <threshold>" << endl;
    }
    char *filename = argv[1];
    const uint32_t frame_num = (uint32_t) stoi(argv[2]);
    const uint32_t rect_x = (uint32_t) stoi(argv[3]);
    const uint32_t rect_y = (uint32_t) stoi(argv[4]);
    const uint32_t width = (uint32_t) stoi(argv[5]);
    const uint32_t height = (uint32_t) stoi(argv[6]);
    const uint32_t threshold = (uint32_t) stoi(argv[7]);
    unique_ptr<h264> decoder = make_unique<h264>(filename);

    auto frame = decoder->load_frame(frame_num);
    /* check if the indicated region is within the frame */
    if (rect_y + height > frame.height() || rect_x + width > frame.width())
        throw std::runtime_error("rectangle is outside the range");

    /* de-reference shared_ptr as the vector will be copied */
    CropOperator crop(rect_x / 16, rect_y / 16, width / 16, height / 16,
                      frame);
    ThresholdOperator thresh(threshold, crop);
    thresh.execute();
    cout << "Motion in selected region: " << (thresh.result() ? "true"
                                                              : "false")
         << endl;
}