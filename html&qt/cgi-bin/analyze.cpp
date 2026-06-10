#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "utils.h"
#include "db.h"

volatile bool g_led_running = false;

void* led_waterflow(void* arg)
{
    int fd = *(int*)arg;
    while (g_led_running) {
        for (int j = 0; j < 4 && g_led_running; j++) {
            ioctl(fd, 1, j);
            usleep(200000);
            ioctl(fd, 0, j);
        }
    }
    return NULL;
}

int main()
{
    std::string method;
    std::string queryString;
    std::string postData;

    const char* requestMethod = getenv("REQUEST_METHOD");
    if (requestMethod) method = requestMethod;

    if (method == "POST") {
        const char* contentLength = getenv("CONTENT_LENGTH");
        int len = contentLength ? atoi(contentLength) : 0;
        if (len > 0 && len < 1024 * 1024) {
            postData.resize(len);
            std::cin.read(&postData[0], len);
        }
    } else {
        const char* qs = getenv("QUERY_STRING");
        if (qs) queryString = qs;
    }

    std::map<std::string, std::string> params;
    if (method == "POST") {
        params = parsePostData(postData);
    } else {
        params = parsePostData(queryString);
    }

    std::string ip     = params.count("ip")     ? params["ip"]     : "";
    std::string prompt = params.count("prompt") ? params["prompt"] : "";

    if (ip.empty()) {
        std::cout << "Content-Type: text/html; charset=utf-8\r\n\r\n";
        std::cout << R"(<!DOCTYPE html><html><head><meta charset="UTF-8"></head><body>)";
        std::cout << R"(<script>parent.displayResult({"status":"error","message":"请输入PC IP地址"});</script>)";
        std::cout << "</body></html>";
        return 0;
    }

    if (prompt.empty()) {
        prompt = "请简要描述当前画面。";
    }

    std::string body = "{\"prompt\":\"" + escapeJson(prompt) + "\",\"return_image\":true}";

    int ledFd = open("/dev/ledtest", 0);
    pthread_t tid;

    if (ledFd >= 0) {
        g_led_running = true;
        pthread_create(&tid, NULL, led_waterflow, &ledFd);
    }

    std::string response = httpPost(ip, 8000, "/analyze", body, 30);

    if (ledFd >= 0) {
        g_led_running = false;
        pthread_join(tid, NULL);
        for (int j = 0; j < 4; j++) {
            ioctl(ledFd, 1, j);
        }
        close(ledFd);
    }

    bool    requestOk = false;
    std::string resultText;
    double costTime = 0.0;
    std::string errorMsg;
    std::string imageBase64;

    if (response.empty()) {
        errorMsg = "无法连接到PC端分析服务，请检查IP地址和服务状态";
    } else {
        std::string status = jsonGetString(response, "status");

        if (status == "ok") {
            requestOk = true;
            resultText = jsonGetString(response, "result");
            costTime = jsonGetDouble(response, "cost_time");
            imageBase64 = jsonGetString(response, "image_base64");
        } else {
            errorMsg = jsonGetString(response, "message");
            if (errorMsg.empty()) errorMsg = "未知错误";
        }
    }

    Database db;
    std::string dbPath = DB_PATH;
    const char* customPath = getenv("DB_PATH");
    if (customPath && customPath[0]) dbPath = customPath;
    if (db.open(dbPath)) {
        std::string saveResult = requestOk ? resultText : ("ERROR: " + errorMsg);
        db.insertRecord(ip, prompt, saveResult, costTime, imageBase64);
    }

    std::ostringstream jsonOut;
    jsonOut << "{";
    if (requestOk) {
        jsonOut << "\"status\":\"ok\"";
        jsonOut << ",\"result\":\"" << escapeJson(resultText) << "\"";
        jsonOut << ",\"cost_time\":" << costTime;
        if (!imageBase64.empty()) {
            jsonOut << ",\"image\":\"" << escapeJson(imageBase64) << "\"";
        }
    } else {
        jsonOut << "\"status\":\"error\"";
        jsonOut << ",\"message\":\"" << escapeJson(errorMsg) << "\"";
    }
    jsonOut << "}";

    std::string jsonStr = jsonOut.str();

    std::cout << "Content-Type: text/html; charset=utf-8\r\n";
    std::cout << "Content-Length: " << (jsonStr.size() + 100) << "\r\n";
    std::cout << "\r\n";
    std::cout << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body>";
    std::cout << "<script>parent.displayResult(" << jsonStr << ");</script>";
    std::cout << "</body></html>";

    return 0;
}
