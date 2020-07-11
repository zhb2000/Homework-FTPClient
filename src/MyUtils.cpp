#include "../include/MyUtils.h"
#include <regex>

namespace utils
{

    std::pair<int, int> getPortFromStr(const std::string &msg)
    {
        std::regex e(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
        std::sregex_iterator iter(msg.begin(), msg.end(), e);
        int p1 = std::stoi((*iter)[5].str());
        int p2 = std::stoi((*iter)[6].str());
        return {p1, p2};
    }

} // namespace utils
