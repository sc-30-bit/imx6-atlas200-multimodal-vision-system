import os
import io
import time
import base64

import cv2
import torch
from PIL import Image
from fastapi import FastAPI
from pydantic import BaseModel
from transformers import AutoModel, AutoTokenizer


# =========================
# 基本配置
# =========================

MODEL_PATH = r"E:\github_project\IoT\models\MiniCPM-V-4-int4"

# 测试阶段：先用本地图片
TEST_IMAGE_PATH = r"C:\Users\LENOVO\Pictures\Camera Roll\OIP.jpg"

# 接 MediaMTX 时 True
USE_RTSP = True

# PC 本机读取 MediaMTX 流
RTSP_URL = "rtsp://127.0.0.1:8554/live"

# 输入给 MiniCPM 的图像尺寸
IMAGE_SIZE = 448

# 返回给 IMX6 显示的 JPEG 质量
RETURN_JPEG_QUALITY = 80

# 生成长度
MAX_NEW_TOKENS = 128

# OpenCV/FFmpeg 低延迟 RTSP 参数
# 必须在 VideoCapture 创建前设置
os.environ["OPENCV_FFMPEG_CAPTURE_OPTIONS"] = (
    "rtsp_transport;tcp|"
    "fflags;nobuffer|"
    "flags;low_delay|"
    "max_delay;0|"
    "reorder_queue_size;0"
)


# =========================
# FastAPI 初始化
# =========================

app = FastAPI(title="MiniCPM-V Analyze Service")


class AnalyzeRequest(BaseModel):
    prompt: str = ""
    return_image: bool = True


# =========================
# 图像处理
# =========================

def letterbox_pil(image: Image.Image, size: int = 640, color=(114, 114, 114)) -> Image.Image:
    """
    保持比例缩放，并补边到 size x size。
    避免直接 resize 导致图像变形。
    """
    image = image.convert("RGB")
    w, h = image.size

    scale = min(size / w, size / h)
    new_w = int(w * scale)
    new_h = int(h * scale)

    resized = image.resize((new_w, new_h), Image.BILINEAR)

    canvas = Image.new("RGB", (size, size), color)
    left = (size - new_w) // 2
    top = (size - new_h) // 2
    canvas.paste(resized, (left, top))

    return canvas


def pil_to_base64_jpeg(image: Image.Image, quality: int = 80) -> str:
    """
    把 PIL Image 编码成 base64 JPEG 字符串，给 IMX6 Qt 显示。
    """
    image = image.convert("RGB")

    buf = io.BytesIO()
    image.save(buf, format="JPEG", quality=quality)
    return base64.b64encode(buf.getvalue()).decode("utf-8")


def load_test_image() -> Image.Image:
    image = Image.open(TEST_IMAGE_PATH).convert("RGB")
    image = letterbox_pil(image, IMAGE_SIZE)
    return image


def capture_frame_from_rtsp(rtsp_url: str) -> Image.Image:
    """
    从 RTSP 视频流中抓取当前帧。
    返回的 image 同时用于 MiniCPM 分析和 IMX6 显示，保证二者对应同一帧。
    """
    cap = cv2.VideoCapture(rtsp_url, cv2.CAP_FFMPEG)

    if not cap.isOpened():
        raise RuntimeError(f"无法打开 RTSP 视频流: {rtsp_url}")

    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    frame = None

    # 多读几帧，尽量拿到较新的帧
    for _ in range(8):
        ret, img = cap.read()
        if ret and img is not None:
            frame = img

    cap.release()

    if frame is None:
        raise RuntimeError("无法从 RTSP 视频流读取图像帧")

    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    image = Image.fromarray(frame_rgb)

    # MiniCPM 输入和 IMX6 显示都用这一张 letterbox 后的图
    image = letterbox_pil(image, IMAGE_SIZE)

    return image


def get_current_image() -> Image.Image:
    if USE_RTSP:
        return capture_frame_from_rtsp(RTSP_URL)
    else:
        return load_test_image()


# =========================
# 加载 MiniCPM-V
# =========================

print("Loading MiniCPM-V tokenizer...")

tokenizer = AutoTokenizer.from_pretrained(
    MODEL_PATH,
    trust_remote_code=True
)

print("Loading MiniCPM-V model...")

model = AutoModel.from_pretrained(
    MODEL_PATH,
    trust_remote_code=True,
    attn_implementation="sdpa",
    torch_dtype=torch.bfloat16
)

model = model.eval().cuda()

print("MiniCPM-V service is ready.")


# =========================
# API 接口
# =========================

@app.get("/")
def root():
    return {
        "status": "ok",
        "message": "MiniCPM-V analyze service is running",
        "use_rtsp": USE_RTSP,
        "rtsp_url": RTSP_URL if USE_RTSP else None,
        "image_size": IMAGE_SIZE
    }



@app.get("/snapshot")
def snapshot():
    try:
        image = get_current_image()
        image_base64 = pil_to_base64_jpeg(image, quality=60)

        return {
            "status": "ok",
            "image_base64": image_base64
        }

    except Exception as e:
        return {
            "status": "error",
            "message": str(e)
        }

@app.post("/analyze")
def analyze(req: AnalyzeRequest):
    start_time = time.time()

    user_prompt = req.prompt.strip()

    if not user_prompt:
        user_prompt = "请简要描述当前画面。"

    try:
        image = get_current_image()

        final_prompt = (
            "你是一个嵌入式视觉分析助手。"
            "请基于当前图像回答用户问题。"
            "注意一定回答是否有火灾、交通事故、人摔倒等异常情况。"
            "回答要简洁、明确，尽量控制在100字以内。\n"
            f"用户问题：{user_prompt}"
        )

        msgs = [
            {
                "role": "user",
                "content": [image, final_prompt]
            }
        ]

        with torch.inference_mode():
            answer = model.chat(
                msgs=msgs,
                image=image,
                tokenizer=tokenizer,
                max_new_tokens=MAX_NEW_TOKENS,
                sampling=False
            )

        cost_time = time.time() - start_time

        image_base64 = ""
        if req.return_image:
            image_base64 = pil_to_base64_jpeg(image, RETURN_JPEG_QUALITY)

        return {
            "status": "ok",
            "result": answer,
            "cost_time": round(cost_time, 3),
            "image_base64": image_base64
        }

    except Exception as e:
        return {
            "status": "error",
            "message": str(e)
        }


@app.post("/analyze_debug")
def analyze_debug(req: AnalyzeRequest):
    """
    调试接口：功能同 analyze，但 prompt 不加系统前缀。
    """
    start_time = time.time()

    user_prompt = req.prompt.strip()

    if not user_prompt:
        user_prompt = "请描述这张图片。"

    try:
        image = get_current_image()

        msgs = [
            {
                "role": "user",
                "content": [image, user_prompt]
            }
        ]

        with torch.inference_mode():
            answer = model.chat(
                msgs=msgs,
                image=image,
                tokenizer=tokenizer,
                max_new_tokens=MAX_NEW_TOKENS,
                sampling=False
            )

        cost_time = time.time() - start_time

        image_base64 = ""
        if req.return_image:
            image_base64 = pil_to_base64_jpeg(image, RETURN_JPEG_QUALITY)

        return {
            "status": "ok",
            "result": answer,
            "cost_time": round(cost_time, 3),
            "image_base64": image_base64
        }

    except Exception as e:
        return {
            "status": "error",
            "message": str(e)
        }

