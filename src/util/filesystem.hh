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
#include <dirent.h>
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


inline bool has_suffix(const std::string &s, const std::string &suffix) {
    return (s.size() >= suffix.size())
           && equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

inline std::vector<std::string> get_files(const std::string &dir_name,
                                          const std::string &ext) {
    std::vector<std::string> result;
    DIR *dir = opendir(dir_name.c_str());
    if(!dir) {
        return result;
    } else {
        dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (has_suffix(entry->d_name, ext)) {
                result.emplace_back(entry->d_name);
            }
        }
        return result;
    }
}


inline std::string fs_basename(const std::string &name) {
    char sep = '/';

#ifdef _WIN32
        sep = '\\';
#endif
        size_t i = name.rfind(sep, name.length());
        if (i != std::string::npos) {
            return(name.substr(i+1, name.length() - i));
        }
        return("");
}

#endif //H264FLOW_FILESYSTEM_HH
