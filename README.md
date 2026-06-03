# 基于 IMX6 与 Atlas 200 DK 的嵌入式多模态视觉交互分析系统

本项目为 HIT 嵌入式/物联网相关课程设计项目，实现了一个基于局域网四端协同的嵌入式多模态视觉交互分析系统。系统结合 Atlas 200 DK 的边缘目标检测能力、PC 端多模态大模型分析能力、IMX6 实验箱的嵌入式交互与外设控制能力，实现了从摄像头采集、目标检测、RTSP 推流、当前帧抓取、多模态图像分析，到 Qt/Web 端结果展示与历史记录保存的完整流程。

## 项目名称

**基于 IMX6 及 Atlas 200 DK 的嵌入式多模态交互分析系统**

## 系统架构

系统采用四端协同架构，所有设备通过交换机连接在同一局域网内。

| 设备 | IP 地址 | 功能 |
|---|---|---|
| Atlas 200 DK | `192.168.137.100` | USB 摄像头采集、YOLO26n 目标检测/跟踪、RTSP 推流 |
| PC1 | `192.168.137.101` | MediaMTX 流媒体服务器、MiniCPM-V-4-int4 多模态分析服务 |
| IMX6 实验箱 | `192.168.137.103` | Qt 客户端、Boa Web 服务、SQLite 历史记录、LED 灯阵列控制 |
| PC2 Ubuntu 虚拟机 | `192.168.137.105` | 通过 Firefox 浏览器访问 IMX6 Web 页面 |

## 核心功能

- Atlas 200 DK 端部署 YOLO26n 目标检测模型；
- 支持 `YOLO26n.pt → YOLO26n.onnx → YOLO26n.om` 模型转换；
- 基于华为 CANN 工具链进行 FP16 推理；
- 使用 AIPP 静态预处理完成输入归一化；
- Atlas 端通过 GStreamer/H.264 向 PC1 推送 RTSP 视频流；
- PC1 运行 MediaMTX 流媒体服务器；
- PC1 运行 MiniCPM-V-4-int4 多模态分析服务；
- IMX6 Qt 客户端支持当前画面显示、提示词输入、预设提示词、分析结果显示和历史记录；
- IMX6 Web 端支持浏览器交互和 SQLite 历史记录保存；
- LED 灯阵列显示系统运行状态和分析状态。

## 系统流程

1. Atlas 200 DK 外接 USB 单目摄像头采集实时画面；
2. Atlas 端使用 YOLO26n.om 对画面进行实时目标检测/跟踪；
3. 检测后画面通过 RTSP 推送到 PC1 的 MediaMTX 服务；
4. IMX6 Qt/Web 端向 PC1 发送提示词和分析请求；
5. PC1 从 RTSP 流中抓取当前帧；
6. MiniCPM-V-4-int4 根据当前帧和提示词进行多模态分析；
7. PC1 返回 `image_base64`、`result` 和 `cost_time`；
8. IMX6 Qt/Web 端显示当前帧、分析结果和历史记录；
9. Web 端将历史记录保存到 SQLite 数据库；
10. LED 灯阵列根据系统状态进行流水显示和分析提示。

## Atlas 200 DK 模型部署

YOLO26n 模型部署流程：

```text
YOLO26n.pt
    ↓
YOLO26n.onnx
    ↓
YOLO26n.om
    ↓
Atlas 200 DK / Ascend 310B4 推理

AIPP 静态预处理配置示例：

```text
aipp_op {
    aipp_mode: static
    input_format: RGB888_U8
    csc_switch: false
    src_image_size_w: 640
    src_image_size_h: 640

    mean_chn_0: 0
    mean_chn_1: 0
    mean_chn_2: 0
    var_reci_chn_0: 0.003921568627
    var_reci_chn_1: 0.003921568627
    var_reci_chn_2: 0.003921568627
}
```

## PC1 多模态分析服务

PC1 端运行 FastAPI 服务，主要接口如下：

| 接口               | 方法   | 功能                   |
| ---------------- | ---- | -------------------- |
| `/`              | GET  | 服务状态检测               |
| `/snapshot`      | GET  | 抓取当前帧并返回 Base64 JPEG |
| `/analyze`       | POST | 根据当前帧和提示词进行多模态分析     |
| `/analyze_debug` | POST | 调试接口，不添加系统提示词        |

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

## IMX6 Qt 客户端

Qt 客户端主要功能：

* 定时请求 `/snapshot` 显示当前画面；
* 支持 PC IP 配置；
* 支持服务连接测试；
* 支持提示词输入与修改；
* 支持“描述、交通、风险、计数”等预设提示词；
* 点击分析后请求 `/analyze`；
* 显示 MiniCPM-V 分析结果与耗时；
* 支持分析历史记录显示。

## Web 与 SQLite

IMX6 上运行 Boa Web 服务器，PC2 可通过 Firefox 浏览器访问 Web 界面。Web 端支持：

* 当前画面显示；
* 提示词输入；
* 多模态分析请求；
* 分析结果显示；
* 历史记录查询；
* SQLite 数据库存储。

SQLite 历史记录表可包含：

| 字段        | 类型      | 说明    |
| --------- | ------- | ----- |
| id        | INTEGER | 记录编号  |
| time      | TEXT    | 分析时间  |
| prompt    | TEXT    | 用户提示词 |
| result    | TEXT    | 分析结果  |
| cost_time | REAL    | 分析耗时  |

## LED 状态显示

IMX6 实验箱 LED 灯阵列用于显示系统状态：

* Web 服务启动后，LED 流水式滚动显示“嵌入式多模态交互分析系统”；
* 点击分析后，4 个 LED 灯依次亮起；
* 分析结束后，LED 灯熄灭。

## 技术栈

* **嵌入式平台**：IMX6 实验箱、Atlas 200 DK
* **AI 推理**：YOLO26n、MiniCPM-V-4-int4
* **昇腾部署**：CANN、ATC、ACL Runtime、AIPP
* **视频流**：RTSP、MediaMTX、GStreamer、H.264
* **交互界面**：Qt、Boa Web、Firefox
* **服务端**：FastAPI、OpenCV、PyTorch、Transformers
* **数据库**：SQLite
* **网络环境**：纯局域网交换机

## 项目特点

* **多端协同**：Atlas 负责检测，PC 负责大模型分析，IMX6 负责交互与控制；
* **边缘部署**：YOLO26n 部署到 Atlas 200 DK，实现端侧实时目标检测；
* **多模态交互**：支持用户通过自然语言提示词对当前画面进行分析；
* **低负载显示**：IMX6 通过 `/snapshot` 获取 Base64 JPEG 当前帧，避免实时 H.264 解码压力；
* **双界面交互**：同时支持 Qt 本地界面和 Web 浏览器界面；
* **嵌入式外设联动**：LED 灯阵列用于系统状态提示；
* **历史记录保存**：Web 端分析记录写入 SQLite 数据库。

## 课程信息

本项目为 HIT 计算机系统设计与实现/嵌入式/物联网相关课程设计项目。

## License

仅用于课程设计、学习与实验展示。

```
```
