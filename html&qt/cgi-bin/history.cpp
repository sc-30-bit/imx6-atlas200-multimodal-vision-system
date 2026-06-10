#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include "db.h"
#include "utils.h"

int main()
{
    Database db;
    std::string dbPath = DB_PATH;
    const char* customPath = getenv("DB_PATH");
    if (customPath && customPath[0]) dbPath = customPath;

    std::vector<AnalysisRecord> records;

    if (db.open(dbPath)) {
        records = db.getAllRecords(50);
    }
    std::ostringstream jsonOut;
    jsonOut << "[";
    for (size_t i = 0; i < records.size(); ++i) {
        const AnalysisRecord& r = records[i];
        if (i > 0) jsonOut << ",";
        jsonOut << "{";
        jsonOut << "\"id\":" << r.id;
        jsonOut << ",\"ip\":\"" << escapeJson(r.ip) << "\"";
        jsonOut << ",\"prompt\":\"" << escapeJson(r.prompt) << "\"";
        jsonOut << ",\"result\":\"" << escapeJson(r.result) << "\"";
        jsonOut << ",\"cost_time\":" << r.cost_time;
        jsonOut << ",\"created_at\":\"" << escapeJson(r.created_at) << "\"";
        if (!r.image.empty()) {
            jsonOut << ",\"image\":\"" << escapeJson(r.image) << "\"";
        }
        jsonOut << "}";
    }
    jsonOut << "]";

    std::string jsonStr = jsonOut.str();

    std::cout << "Content-Type: application/json; charset=utf-8\r\n";
    std::cout << "Content-Length: " << jsonStr.size() << "\r\n";
    std::cout << "Cache-Control: no-cache\r\n";
    std::cout << "\r\n";
    std::cout << jsonStr;

    return 0;
}
