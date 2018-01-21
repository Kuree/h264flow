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

#ifndef H264FLOW_FILESYSTEM_HH
#define H264FLOW_FILESYSTEM_HH


#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

inline bool file_exists(const std::string &filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

inline std::string file_extension(const std::string &filename) {
    auto period = filename.find_last_of('.');
    return filename.substr(period);
}

inline bool dir_exists(const std::string &dir_name) {
    struct stat buffer;
    return (stat(dir_name.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

#endif //H264FLOW_FILESYSTEM_HH
