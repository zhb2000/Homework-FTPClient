#include "../include/MyUtils.h"
#include <memory>
#include <regex>

using std::unique_ptr;

namespace utils
{

    std::pair<std::string, int> getIPAndPortForPSAV(const std::string &msg)
    {
        std::regex e(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
        std::sregex_iterator iter(msg.begin(), msg.end(), e);

        std::string h1, h2, h3, h4;
        h1 = (*iter)[1].str();
        h2 = (*iter)[2].str();
        h3 = (*iter)[3].str();
        h4 = (*iter)[4].str();
        std::string hostname = h1 + "." + h2 + "." + h3 + "." + h4;

        int p1, p2;
        p1 = std::stoi((*iter)[5].str());
        p2 = std::stoi((*iter)[6].str());
        int port = p1 * 256 + p2;

        return {hostname, port};
    }

    int getPortForEPSV(const std::string &msg)
    {
        std::regex e(R"(\(\|\|\|(\d+)\|\))");
        std::sregex_iterator iter(msg.begin(), msg.end(), e);

        int port = std::stoi((*iter)[1]);
        return port;
    }

    int getSizeFromMsg(const std::string &msg)
    {
        std::regex e(R"(^213\s+(\d+))");
        std::sregex_iterator iter(msg.begin(), msg.end(), e);

        int filesize = std::stoi((*iter)[1]);
        return filesize;
    }

    std::string getDirFromMsg(const std::string &msg)
    {
        // "(.+)"
        std::regex e("\"(.+)\"");
        std::sregex_iterator iter(msg.begin(), msg.end(), e);
        std::string dir = (*iter)[1].str();
        return dir;
    }

    int recvAll(SOCKET sock, std::string &recvMsg)
    {
        const int maxlen = 1024;
        unique_ptr<char[]> buf(new char[maxlen]);
        recvMsg.clear();
        bool lessThanZero = false;
        int iResult;
        while (true)
        {
            iResult = recv(sock, buf.get(), maxlen, 0);
            if (iResult > 0)
                recvMsg += std::string(buf.get(), iResult);
            else if (iResult == 0)
                break;
            else if (iResult < 0)
            {
                lessThanZero = true;
                break;
            }
        }
        if (lessThanZero && recvMsg.length() == 0)
            return -1;
        else
            return int(recvMsg.length());
    }

    long long getFilesize(std::ifstream &ifs)
    {
        auto currentPos = ifs.tellg(); //当前位置
        ifs.seekg(0, std::ios::end);   //移动到文件流结尾
        auto size = ifs.tellg();
        ifs.seekg(currentPos); //恢复位置
        return size;
    }

} // namespace utils
