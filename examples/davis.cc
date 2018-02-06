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

#include <vector>
#include "../src/util/argparser.hh"
#include "../src/model/model-io.hh"

using namespace std;

int main(int argc, char * argv[]) {
    ArgParser parser("Read DAVIS annotation png file");
    parser.add_arg("-i", "input", "DAVIS annotation file");
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();
    string input_file = arg_values["input"];

    map<int, int> labels {{0, 0}};
    auto label_maps = load_davis_annotation(input_file);
    auto maps = label_maps.first;
    for (uint32_t i = 0; i < maps.size(); i += 10) {
        for (uint32_t j = 0; j < maps[i].size(); j += 10) {
            int label = maps[i][j];
            if (labels.find(label) != labels.end()) {
                if (label)
                    cout << (uint8_t)(0x41 + labels[label]);
                else
                    cout << " ";
            } else {
                labels.insert(make_pair(label, labels.size()));
                cout << (uint8_t)(0x41 + labels[label]);
            }
        }
        cout << endl;
    }
    return EXIT_SUCCESS;
}