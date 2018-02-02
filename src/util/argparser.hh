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

#ifndef H264FLOW_ARGPARSER_HH
#define H264FLOW_ARGPARSER_HH


/* header only, Python-style ArgParser implementation */

#include <iostream>
#include <map>
#include <iomanip>
#include <set>

class ArgParser {
public:
    explicit ArgParser(const std::string &desc) : _desc(desc),
                                                  _args(), _values(), _keys() {}
    bool parse(int argc, char* argv[]);
    void add_arg(const std::string & arg, const std::string &dst,
                 const std::string &desc, bool required = true);
    std::map<std::string, std::string> get_args();

    void print_help(const std::string &program_name);

private:
    struct Argument {
        std::string name;
        std::string dst;
        std::string desc;
        bool required;
    };
    std::string _desc;
    std::vector<ArgParser::Argument> _args;
    std::map<std::string, std::string> _values;

    /* only to find duplicated keys */
    std::set<std::string> _keys;
};

bool ArgParser::parse(int argc, char **argv) {
    int i = 1;
    while (i < argc) {
        if (argv[i][0] == '-') {
            std::string key = argv[i];
            std::string value = argv[++i];
            _values.insert({key, value});
        } else {
            std::cerr << "ERROR: parsing at " << argv[i] << std::endl;
            print_help(argv[0]);
            return false;
        }
        ++i;
    }

    /* check if any required is missing */
    for (auto const & argument : _args) {
        if (argument.required) {
            if (_values.find(argument.name) == _values.end()) {
                std::cerr << "ERROR: " << argument.name << "not found"
                          << std::endl;
                print_help(argv[0]);
                return false;
            }
        }
    }
    return true;
}

void ArgParser::add_arg(const std::string &arg, const std::string &dst,
                        const std::string &desc, bool required) {

    if (arg[0] != '-') throw std::runtime_error("arg has to start with a '-'");
    if (_keys.find(arg) != _keys.end())
        throw std::runtime_error(arg + " has been already added");
    ArgParser::Argument argument {
        arg, dst, desc, required
    };
    _args.emplace_back(argument);
    _keys.insert(arg);
}


void ArgParser::print_help(const std::string &program_name) {
    std::cerr << _desc << std::endl << "Usage: " << program_name << " ";
    for (const auto &x : _args) {
        if (!x.required) std::cerr << "[";
        std::cerr << x.name << " <" << x.dst << ">";
        if (!x.required)
            std::cerr << "] ";
        else
            std::cerr << " ";
    }
    std::cerr << std::endl;
    for (const auto &x : _args) {
        std::cerr << std::setw(12) << x.name << "    " << x.desc
                  << std::endl;
    }
}

std::map<std::string, std::string> ArgParser::get_args() {
    std::map<std::string, std::string> result;
    for (const auto &arg : _args) {
        if (_values.find(arg.name) != _values.end())
            result.insert({arg.dst, _values[arg.name]});
    }
    return result;
}

#endif //H264FLOW_ARGPARSER_HH
