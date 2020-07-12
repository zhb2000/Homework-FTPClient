#include "../include/MyUtils.h"
#include <regex>

namespace utils
{

    std::pair<std::string, int> getIPAndPortForPSAV(const std::string &msg)
    {
        std::regex e(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
        std::sregex_iterator iter(msg.begin(), msg.end(), e);

        std::string h1, h2, h3, h4;
        h1 = std::stoi((*iter)[1].str());
        h2 = std::stoi((*iter)[2].str());
        h3 = std::stoi((*iter)[3].str());
        h4 = std::stoi((*iter)[4].str());
        std::string hostname = h1 + "." + h2 + "." + h3 + "." + h4;

        int p1, p2;
        p1 = std::stoi((*iter)[5].str());
        p2 = std::stoi((*iter)[6].str());
        int port = p1 * 256 + p2;

        return {hostname, port};
    }

} // namespace utils
