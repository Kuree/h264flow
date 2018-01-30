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

#include <algorithm>
#include "../src/query/operator.hh"
#include "../src/util/argparser.hh"

using namespace std;

int main(int argc, char *argv[]) {
    ArgParser parser("Read motion vectors from a media file "
                             "and visualize them");
    parser.add_arg("-i", "input", "media file input");
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();

    string filename = arg_values["input"];

    /* open decoder */
    unique_ptr<h264> decoder = make_unique<h264>(filename);

    for (uint32_t frame_counter = 0; frame_counter < decoder->index_size();
         frame_counter++) {
        try {
            decoder->load_frame(frame_counter);
        } catch (std::runtime_error &ex) {
            cerr << "unable to decode frame " << frame_counter << endl;
        }
    }
    return EXIT_SUCCESS;
}
