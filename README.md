# 基于 IMX6 与 Atlas 200 DK 的嵌入式多模态视觉交互分析系统

本项目实现了一个局域网内多端协同的嵌入式多模态视觉分析系统。Atlas 200 DK 负责摄像头采集、YOLO26n 目标检测和 RTSP 推流；PC 负责 MediaMTX 流媒体转发和 MiniCPM-V 图文分析；IMX6 实验箱负责 Qt/Web 交互、SQLite 历史记录和 LED/点阵状态显示。

Atlas 200 DK 端 YOLO26 CANN C++ 推理与部署代码参考：[sc-30-bit/yolo26-cann-cpp](https://github.com/sc-30-bit/yolo26-cann-cpp)。

## 系统架构

![系统整体架构图](figs/overview.png)

| 设备 | 示例 IP | 功能 |
| --- | --- | --- |
| Atlas 200 DK | `192.168.137.100` | USB 摄像头采集、YOLO26n 检测、RTSP 推流 |
| PC | `192.168.137.101` | MediaMTX、MiniCPM-V-4-int4、FastAPI 分析服务 |
| IMX6 实验箱 | `192.168.137.103` | Qt 客户端、Boa Web、CGI、SQLite、LED/点阵控制 |
| 浏览器端 | `192.168.137.105` | 访问 IMX6 Web 页面 |

数据流：

```text
摄像头
  -> Atlas 200 DK：YOLO26n 检测与画面绘制
  -> RTSP 推流到 PC MediaMTX
  -> PC FastAPI 抓取当前帧并调用 MiniCPM-V
  -> IMX6 Qt/Web 显示图像、分析结果和历史记录
```

## 仓库结构

```text
.
├── README.md
├── figs/
│   └── overview.png
├── MiniCPM&MediaMTX/
│   ├── app_minicpm.py
│   ├── requirements.txt
│   ├── README.md
│   ├── mediamtx_v1.17.1_windows_amd64/
│   │   └── mediamtx.yml
│   └── models/
│       └── MiniCPM-V-4-int4/        # 本地模型目录，按说明下载
└── html&qt/
    ├── Makefile
    ├── html/
    │   └── index.html
    ├── cgi-bin/
    │   ├── analyze.cpp
    │   ├── history.cpp
    │   ├── dotmatrix.cpp
    │   ├── db.h
    │   └── utils.h
    └── imx6_qt_client/
        ├── main.cpp
        ├── mainwindow.cpp
        ├── mainwindow.h
        ├── mainwindow.ui
        └── imx6_qt_client.pro
```

## PC 端部署

PC 端运行 `MediaMTX + FastAPI + MiniCPM-V-4-int4`，用于接收 Atlas 视频流并响应 IMX6 的分析请求。

安装依赖：

```powershell
cd '.\MiniCPM&MediaMTX'
python -m pip install -r requirements.txt
```

MiniCPM-V-4-int4 模型权重不随仓库提供，请下载到本地模型目录：

```powershell
hf auth login
hf download openbmb/MiniCPM-V-4-int4 --local-dir '.\MiniCPM&MediaMTX\models\MiniCPM-V-4-int4'
```

`app_minicpm.py` 中的 `MODEL_PATH` 需要指向本地模型目录。

MediaMTX 可从官方发布页下载，仓库中保留了 `mediamtx.yml` 配置文件。启动后监听 RTSP `8554` 端口：

```powershell
cd '.\MiniCPM&MediaMTX\mediamtx_v1.17.1_windows_amd64'
.\mediamtx.exe
```

启动 MiniCPM-V 分析服务：

```powershell
cd '.\MiniCPM&MediaMTX'
python -m uvicorn app_minicpm:app --host 0.0.0.0 --port 8000
```

主要接口：

| 接口 | 方法 | 功能 |
| --- | --- | --- |
| `/` | GET | 服务状态检测 |
| `/snapshot` | GET | 抓取当前帧，返回 Base64 JPEG |
| `/analyze` | POST | 根据当前帧和提示词进行图文分析 |
| `/analyze_debug` | POST | 调试接口 |

`/analyze` 请求示例：

```json
{
  "prompt": "请判断当前画面中是否有车辆或行人，只输出一句话。",
  "return_image": true
}
```

返回示例：

```json
{
  "status": "ok",
  "result": "当前画面中有车辆和行人，未发现明显异常风险。",
  "cost_time": 8.632,
  "image_base64": "..."
}
```

## Atlas 200 DK 端部署

Atlas 200 DK 端负责摄像头采集、YOLO26n 推理、检测结果绘制和 RTSP 推流。YOLO26 CANN C++ 部署流程参考：[sc-30-bit/yolo26-cann-cpp](https://github.com/sc-30-bit/yolo26-cann-cpp)。

模型转换流程：

```text
YOLO26n.pt -> YOLO26n.onnx -> YOLO26n.om -> Atlas 200 DK 推理
```

推流目标为 PC 的 MediaMTX：

```text
rtsp://192.168.137.101:8554/live
```

Atlas 端需要使用 PC 的局域网 IP，不能使用 `127.0.0.1`。

## IMX6 Qt 客户端

Qt 客户端位于：

```text
html&qt/imx6_qt_client/
```

主要功能：

- 配置 PC IP。
- 每秒请求 `/snapshot` 刷新当前帧。
- 输入或选择预设提示词。
- 调用 `/analyze` 获取 MiniCPM-V 分析结果。
- 显示返回图像、结果文本和推理耗时。
- 保存本地分析历史。

Qt 端访问：

```text
http://<PC_IP>:8000/
http://<PC_IP>:8000/snapshot
http://<PC_IP>:8000/analyze
```

## IMX6 Web 端

Web 端由 Boa + CGI 实现，页面和 CGI 位于：

```text
html&qt/html/index.html
html&qt/cgi-bin/
```

| CGI | 功能 |
| --- | --- |
| `analyze.cgi` | 调用 PC `/analyze`，控制 LED，写入 SQLite |
| `history.cgi` | 查询最近 50 条分析记录 |
| `dotmatrix.cgi` | 控制点阵滚动显示 |

SQLite 默认路径：

```text
/mnt/boa/database/analysis.db
```

交叉编译 CGI：

```sh
cd 'html&qt'
make
```

部署目录示例：

```text
/mnt/boa/
├── html/index.html
├── cgi-bin/analyze.cgi
├── cgi-bin/history.cgi
├── cgi-bin/dotmatrix.cgi
└── database/analysis.db
```

浏览器访问：

```text
http://192.168.137.103/
```

## 启动顺序

1. 将 Atlas、PC、IMX6 接入同一局域网。
2. PC 启动 MediaMTX。
3. Atlas 启动 YOLO26n 检测程序并推流到 `rtsp://192.168.137.101:8554/live`。
4. PC 启动 FastAPI + MiniCPM-V 服务。
5. IMX6 启动 Qt 客户端或 Boa Web 服务。
6. 在 Qt/Web 中填写 PC IP，输入提示词并发起分析。

## 课程说明

本项目用于展示边缘视觉推理、多模态大模型分析、嵌入式人机交互和多设备网络协同的综合实现。
