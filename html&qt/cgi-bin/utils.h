#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>

inline std::string urlDecode(const std::string& src)
{
    std::string result;
    result.reserve(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '%' && i + 2 < src.size()) {
            int high = src[i + 1];
            int low  = src[i + 2];
            int h = (high >= '0' && high <= '9') ? (high - '0')
                  : (high >= 'A' && high <= 'F') ? (high - 'A' + 10)
                  : (high >= 'a' && high <= 'f') ? (high - 'a' + 10) : -1;
            int l = (low >= '0' && low <= '9') ? (low - '0')
                  : (low >= 'A' && low <= 'F') ? (low - 'A' + 10)
                  : (low >= 'a' && low <= 'f') ? (low - 'a' + 10) : -1;
            if (h >= 0 && l >= 0) {
                result += static_cast<char>((h << 4) | l);
                i += 2;
                continue;
            }
        } else if (src[i] == '+') {
            result += ' ';
            continue;
        }
        result += src[i];
    }
    return result;
}

inline std::map<std::string, std::string> parsePostData(const std::string& data)
{
    std::map<std::string, std::string> params;
    std::istringstream stream(data);
    std::string pair;
    while (std::getline(stream, pair, '&')) {
        size_t eqPos = pair.find('=');
        if (eqPos == std::string::npos) continue;
        std::string key = pair.substr(0, eqPos);
        std::string val = pair.substr(eqPos + 1);
        params[urlDecode(key)] = urlDecode(val);
    }
    return params;
}

inline std::string escapeJson(const std::string& s)
{
    std::string result;
    result.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;      break;
        }
    }
    return result;
}

inline std::string jsonGetString(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos += search.size();
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";

    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
        ++pos;

    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        ++pos;
        std::string result;
        while (pos < json.size()) {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                ++pos;
                switch (json[pos]) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    default:   result += json[pos]; break;
                }
                ++pos;
                continue;
            }
            if (json[pos] == '"') break;
            result += json[pos];
            ++pos;
        }
        return result;
    }

    size_t end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']')
        ++end;

    std::string val = json.substr(pos, end - pos);
    while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\n' || val.back() == '\r'))
        val.pop_back();
    while (!val.empty() && (val.front() == ' ' || val.front() == '\t'))
        val.erase(0, 1);

    return val;
}

inline double jsonGetDouble(const std::string& json, const std::string& key)
{
    std::string val = jsonGetString(json, key);
    if (val.empty()) return 0.0;
    return atof(val.c_str());
}

inline std::string base64Encode(const std::string& data)
{
    static const char* table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
        unsigned char a = static_cast<unsigned char>(data[i]);
        unsigned char b = (i + 1 < data.size()) ? static_cast<unsigned char>(data[i + 1]) : 0;
        unsigned char c = (i + 2 < data.size()) ? static_cast<unsigned char>(data[i + 2]) : 0;

        result += table[a >> 2];
        result += table[((a & 0x03) << 4) | (b >> 4)];
        result += (i + 1 < data.size()) ? table[((b & 0x0f) << 2) | (c >> 6)] : '=';
        result += (i + 2 < data.size()) ? table[c & 0x3f] : '=';
    }
    return result;
}

inline std::string httpPost(const std::string& host, int port,
                            const std::string& path, const std::string& body,
                            int timeoutSec = 15)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "";

    struct hostent* server = gethostbyname(host.c_str());
    if (!server) {
        close(sock);
        return "";
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(port);

    struct timeval tv;
    tv.tv_sec = timeoutSec;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return "";
    }

    std::ostringstream request;
    request << "POST " << path << " HTTP/1.0\r\n";
    request << "Host: " << host << ":" << port << "\r\n";
    request << "Content-Type: application/json; charset=utf-8\r\n";
    request << "Content-Length: " << body.size() << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";
    request << body;

    std::string reqStr = request.str();
    if (send(sock, reqStr.c_str(), reqStr.size(), 0) < 0) {
        close(sock);
        return "";
    }

    std::string response;
    char buf[4096];
    int n;
    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        response += buf;
    }
    close(sock);

    if (response.empty()) return "";

    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos) return "";

    return response.substr(headerEnd + 4);
}

#endif
