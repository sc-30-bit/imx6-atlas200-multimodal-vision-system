# PC 端部署说明

PC 端负责运行 **MediaMTX + MiniCPM-V 4.0 int4 + FastAPI**，用于接收 Atlas 200 推送的视频流，并根据 IMX6 Qt 端发送的文字提示完成图像分析。

## 1. 功能流程

```text
Atlas 200
  ↓ RTSP 推流

PC MediaMTX
  ↓ 读取当前帧

PC FastAPI + MiniCPM-V 4.0 int4
  ↓ HTTP JSON

IMX6 Qt
````

## 2. 目录结构

```text
IoT
├── app_minicpm.py
├── requirements.txt
├── models
│   └── MiniCPM-V-4-int4
└── mediamtx_v1.17.1_windows_amd64
    ├── mediamtx.exe
    └── mediamtx.yml
```

## 3. 环境依赖

推荐环境：

```text
Windows
RTX 4060 8GB
Python 3.10
PyTorch CUDA
FastAPI
MediaMTX
MiniCPM-V 4.0 int4
```

安装依赖：

```powershell
E:/conda/envs_dirs/minicpm/python.exe -m pip install -r requirements.txt
```

## 4. 下载 MiniCPM-V 4.0 int4

```powershell
hf auth login
```

```powershell
hf download openbmb/MiniCPM-V-4-int4 --local-dir E:\github_project\IoT\models\MiniCPM-V-4-int4
```

## 5. 配置 MediaMTX

修改 `mediamtx.yml` 中的 `paths`：

```yaml
paths:
  live:
    source: publisher
```

启动 MediaMTX：

```powershell
cd E:\github_project\IoT\mediamtx_v1.17.1_windows_amd64
.\mediamtx.exe
```

正常日志应包含：

```text
[RTSP] listener opened on :8554
```

## 6. 测试 RTSP 推流

PC 本机测试推流：

```powershell
ffmpeg -re -f lavfi -i testsrc=size=640x480:rate=25 -c:v libx264 -pix_fmt yuv420p -f rtsp rtsp://127.0.0.1:8554/live
```
ffplay -rtsp_transport tcp -fflags nobuffer -flags low_delay -framedrop -sync ext rtsp://127.0.0.1:8554/live  

使用 VLC 打开：

```text
rtsp://127.0.0.1:8554/live
```

能看到测试画面说明 MediaMTX 正常。

## 7. 配置 FastAPI

`app_minicpm.py` 中主要配置：

```python
MODEL_PATH = r"E:\github_project\IoT\models\MiniCPM-V-4-int4"

USE_RTSP = True
RTSP_URL = "rtsp://127.0.0.1:8554/live"

IMAGE_SIZE = 448
MAX_NEW_TOKENS = 64
```

推荐在 `model.chat()` 中关闭随机采样：

```python
answer = model.chat(
    msgs=msgs,
    image=image,
    tokenizer=tokenizer,
    max_new_tokens=MAX_NEW_TOKENS,
    sampling=False
)
```

## 8. 启动 FastAPI

```powershell
cd E:\github_project\IoT
E:/conda/envs_dirs/minicpm/python.exe -m uvicorn app_minicpm:app --host 0.0.0.0 --port 8000
```

浏览器打开：

```text
http://127.0.0.1:8000/docs
```

测试 `/analyze` 接口：

```json
{
  "prompt": "请描述当前画面，只输出一句话。"
}
```

返回示例：

```json
{
  "status": "ok",
  "result": "画面中显示彩色测试图案和计时标识。",
  "cost_time": 9.069
}
```

## 9. Atlas 推流地址

Atlas 200 推流到 PC 时，需要使用 PC 的局域网 IP：

```text
rtsp://PC_IP:8554/live
```

例如：

```text
rtsp://192.168.1.10:8554/live
```

注意：Atlas 端不能使用 `127.0.0.1`，因为这表示 Atlas 自己。

## 10. IMX6 调用接口

IMX6 Qt 程序访问 PC 的 FastAPI 接口：

```text
http://PC_IP:8000/analyze
```

请求：

```json
{
  "prompt": "请判断当前画面中是否有车辆或行人，只输出一句话。"
}
```

返回：

```json
{
  "status": "ok",
  "result": "画面中存在一辆车辆，未发现明显行人。",
  "cost_time": 9.1
}
```

## 11. 启动顺序

```text
1. 启动 MediaMTX
2. Atlas 200 推流到 rtsp://PC_IP:8554/live
3. PC 使用 VLC 测试 rtsp://127.0.0.1:8554/live
4. 启动 FastAPI + MiniCPM-V 服务
5. IMX6 Qt 调用 http://PC_IP:8000/analyze
```

## 12. 防火墙

如果 Atlas 或 IMX6 无法访问 PC，需要放行端口：

```powershell
netsh advfirewall firewall add rule name="MediaMTX RTSP 8554" dir=in action=allow protocol=TCP localport=8554
```

```powershell
netsh advfirewall firewall add rule name="MiniCPM FastAPI 8000" dir=in action=allow protocol=TCP localport=8000
```

## 13. 当前状态

```text
MiniCPM-V 4.0 int4 部署完成
FastAPI /analyze 接口测试完成
MediaMTX Windows 部署完成
RTSP 本机推流与拉流测试完成
待接入 Atlas 200 实际视频流
```
