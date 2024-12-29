import os
import subprocess

def extract_album_cover(input_file, output_folder):
    # 获取文件名（不带扩展名）
    file_name = os.path.splitext(os.path.basename(input_file))[0]
    # 设置输出图片路径
    output_image_path = os.path.join(output_folder, f"{file_name}.png")

    # ffmpeg 提取专辑封面的命令
    command = [
        "ffmpeg",
        "-i", input_file,        # 输入音频文件
        "-an",                   # 禁用音频流
        "-vcodec", "png",       # 输出为 PNG 格式
        output_image_path         # 输出图片路径
    ]

    # 运行命令
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    if result.returncode == 0:
        print(f"Album cover extracted to: {output_image_path}")
    else:
        print(f"Failed to extract album cover for {input_file}: {result.stderr}")

def extract_covers_in_directory(directory):
    # 遍历目录中的所有音频文件
    for file in os.listdir(directory):
        if file.endswith(".mp3") or file.endswith(".flac") or file.endswith(".m4a"):
            input_file = os.path.join(directory, file)
            extract_album_cover(input_file, directory)

extract_covers_in_directory("./uploads")
