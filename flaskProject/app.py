from flask import Flask, request, Response, jsonify, send_from_directory
import sys
import os
import re
import subprocess
import hashlib

sys.path.append(os.getcwd())
import sql_tool

app = Flask(__name__)

UPLOAD_FOLDER = './uploads'
MAX_CONTENT_LENGTH = 1024 * 1024 * 1024
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = MAX_CONTENT_LENGTH
current_ffmpeg_process = None

pool = sql_tool.ConnectionPool("tcp://127.0.0.1:3306", "root", "shen2003125", "music_users")

# 确保 uploads 目录存在
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

def generate_song_id(file_path):
    hash_func = hashlib.md5()
    with open(file_path, "rb") as f:
        while chunk := f.read(8192):  # 分块读取，适合大文件
            hash_func.update(chunk)
    return hash_func.hexdigest()

def get_audio_duration(file_path):
    """

    :rtype: object
    """
    try:
        # 调用 ffmpeg 获取音频文件的信息
        result = subprocess.run(
            ['ffprobe', '-v', 'error', '-show_entries', 'format=duration', '-of', 'default=noprint_wrappers=1:nokey=1',
             file_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        # 解析结果
        duration = float(result.stdout.strip())
        return duration
    except Exception as e:
        print(f"Error occurred: {e}")
        return None

@app.route('/uploads/<path:folder>/<path:filename>', methods=['GET'])
def serve_audio_file(folder, filename):
    try:
        # 检查文件夹和文件名是否有效
        if not folder or not filename:
            return jsonify({"error": "Invalid folder or filename"}), 400

        # 构造文件路径
        file_path = os.path.join(app.config['UPLOAD_FOLDER'], folder, filename)

        # 检查文件是否存在
        if not os.path.exists(file_path):
            return jsonify({"error": "File not found"}), 404

        # 返回文件
        return send_from_directory(os.path.join(app.config['UPLOAD_FOLDER'], folder), filename, as_attachment=True)

    except Exception as e:
        return jsonify({"error": f"Error processing request: {str(e)}"}), 500

# 提供 .lrc 文件内容
@app.route('/uploads/<path:folder>/<path:filename>/lrc', methods=['GET'])
def serve_file_lrc(folder, filename):
    try:
        # 检查文件夹和文件名是否符合规范
        if not folder or not filename:
            return jsonify({"error": "Invalid folder or filename"}), 400

        # 构造路径到文件夹和 .lrc 文件
        lrc_file_path = os.path.join(app.config['UPLOAD_FOLDER'], folder, filename)

        # 检查文件是否存在
        if not os.path.exists(lrc_file_path):
            return jsonify({"error": "File not found"}), 404

        # 读取 .lrc 文件内容按行返回
        with open(lrc_file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        # 去除每行的换行符并返回 JSON 数组
        json_array = [line.strip() for line in lines]
        return jsonify(json_array)

    except Exception as e:
        return jsonify({"error": f"Error processing request: {str(e)}"}), 500

@app.route('/stream', methods=['POST'])
def stream_file():
    try:
        # 获取 JSON 请求体中的 filename 字段
        request_data = request.get_json()
        if not request_data or 'filename' not in request_data:
            return jsonify({"error": "Missing 'filename' in request."}), 400

        filename = request_data['filename'].strip()
        if not filename:
            return jsonify({"error": "Filename cannot be empty."}), 400

        # 去除文件名后缀
        base_filename = os.path.splitext(filename)[0]

        # 构造路径到同名文件夹
        file_folder = os.path.join(app.config['UPLOAD_FOLDER'], base_filename)
        if not os.path.exists(file_folder):
            return jsonify({"error": "No matching folder found."}), 404

        # 查找文件夹中的音频文件（支持 flac, mp3, ogg）
        valid_audio_extensions = ['.flac', '.mp3', '.ogg']
        audio_files = [f for f in os.listdir(file_folder) if os.path.splitext(f)[1].lower() in valid_audio_extensions]

        # 查找歌词文件（lrc）
        lrc_files = [f for f in os.listdir(file_folder) if f.lower().endswith('.lrc')]

        # 查找专辑封面文件（png）
        album_covers = [f for f in os.listdir(file_folder) if f.lower().endswith('.png')]

        # 检查是否有音频文件
        if not audio_files:
            return jsonify({"error": "No audio files found in the folder."}), 404

        # 假设只处理第一个音频文件
        selected_audio = audio_files[0]
        audio_path = os.path.join(file_folder, selected_audio)
        audio_url = f"http://{request.host}/uploads/{base_filename}/{selected_audio}"

        # 处理歌词文件（如果存在，取第一个）
        lrc_url = None
        if lrc_files:
            selected_lrc = lrc_files[0]
            lrc_url = f"http://{request.host}/uploads/{base_filename}/{selected_lrc}"

        # 处理专辑封面（如果存在，取第一个）
        album_cover_url = None
        if album_covers:
            selected_cover = album_covers[0]
            album_cover_url = f"http://{request.host}/uploads/{base_filename}/{selected_cover}"

        songid = generate_song_id(audio_path)
        # 构造响应 JSON
        response = {
            "message": "Audio stream started successfully",
            "stream_url": audio_url,
            "lrc_url": lrc_url,
            "album_cover_url": album_cover_url,
            "ID": songid
        }

        return jsonify(response), 200

    except Exception as e:
        return jsonify({"error": f"Error processing request: {str(e)}"}), 500


@app.route('/files', methods=['GET'])
def list_files():
    try:
        result = {}

        # 遍历上传文件夹及其子文件夹
        for root, _, files in os.walk(app.config['UPLOAD_FOLDER']):
            for file_name in files:
                # 检查扩展名是否是 flac, mp3, 或 ogg
                if file_name.lower().endswith(('.flac', '.mp3', '.ogg')):
                    file_path = os.path.join(root, file_name)
                    duration = get_audio_duration(file_path)

                    # 存储文件名和时长信息（只返回文件名）
                    if duration is not None:
                        result[file_name] = f"{duration:.2f} seconds"
                    else:
                        result[file_name] = "Error retrieving duration"

        return jsonify(result)
    except Exception as e:
        return jsonify({"error": f"Error reading directory: {str(e)}"}), 500

@app.route('/file', methods=['POST'])
def list_files_with_name():
    try:
        # 获取请求数据
        data = request.get_json()
        if not data or 'filename' not in data:
            return jsonify({"error": "Missing 'name' field in request JSON"}), 400

        name_filter = data['filename'].strip()

        # 检查字段是否为空
        if not name_filter:
            return jsonify({"error": "The 'filename' field cannot be empty"}), 400

        result = {}

        # 遍历 uploads 文件夹的子文件夹
        for root, dirs, files in os.walk(app.config['UPLOAD_FOLDER']):
            folder_name = os.path.basename(root)

            # 检查当前文件夹是否与过滤条件匹配（大小写不敏感）
            if name_filter.lower() in folder_name.lower():
                # 获取当前文件夹中的音频文件
                audio_files = [
                    f for f in files if f.lower().endswith(('.flac', '.ogg', '.mp3'))
                ]

                # 添加音频文件和时长信息到结果
                for file_name in audio_files:
                    file_path = os.path.join(root, file_name)
                    duration = get_audio_duration(file_path)

                    # 使用相对路径作为键
                    relative_path = os.path.relpath(file_path, app.config['UPLOAD_FOLDER'])
                    if duration is not None:
                        result[relative_path] = f"{duration:.2f} seconds"
                    else:
                        result[relative_path] = "Error retrieving duration"

        return jsonify(result)
    except Exception as e:
        return jsonify({"error": f"Error processing request: {str(e)}"}), 500

@app.route('/upload', methods=['POST'])
def upload_file():
    try:
        if 'file' not in request.files:
            return "No file part", 400
        file = request.files['file']
        if file.filename == '':
            return "No selected file", 400

        file_path = os.path.join(UPLOAD_FOLDER, file.filename)
        file.save(file_path)
        return "File uploaded successfully!", 200
    except Exception as e:
        return f"Error uploading file: {str(e)}", 500


@app.route('/download', methods=['POST'])
def download_file():
    try:
        data = request.get_json()
        filename = data.get("filename")

        if not filename:
            return jsonify({"error": "Missing 'filename' parameter"}), 400

        folder_name, _ = os.path.splitext(filename)

        file_path = os.path.join(app.config['UPLOAD_FOLDER'], folder_name, filename)

        if not os.path.exists(file_path):
            return jsonify({"error": "File not found"}), 404

        return send_from_directory(os.path.join(app.config['UPLOAD_FOLDER'], folder_name), filename, as_attachment=True)

    except Exception as e:
        return jsonify({"error": f"Error processing request: {str(e)}"}), 500

@app.route('/users/register', methods=['POST'])
def register_user():
    try:
        data = request.get_json()
        account = data['account']
        password = data['password']
        username = data['username']

        try:
            con = pool.get_connection()
            result = (sql_tool.user_register(con, password, account, username))
            return jsonify({"success": result})
        except Exception as e:
            # 打印日志方便调试
            print(f"Error during login: {e}")
            return jsonify({"error": "Internal Server Error"}), 500
        finally:
            # pool.releaseConnection(con)
            print("finish register")
    except KeyError:
        return "Missing or invalid fields", 400
    except Exception as e:
        return f"Error processing request: {str(e)}", 500


@app.route('/users/login', methods=['POST'])
def login_user():
    try:
        data = request.get_json()
        account = data['account']
        password = data['password']

        try:
            con = pool.get_connection()
            result = (sql_tool.user_login(con, password, account))
            song_list = sql_tool.user_musicpath_list(con, result)
            return jsonify({"success": result, "song_path_list": song_list})
        except Exception as e:
            # 打印日志方便调试
            print(f"Error during login: {e}")
            return jsonify({"error": "Internal Server Error"}), 500
        finally:
            # pool.releaseConnection(con)
            print("finish login")

    except KeyError:
        return "Missing or invalid fields", 400
    except Exception as e:
        return f"Error processing request: {str(e)}", 500


@app.route('/users/add_music', methods=['POST'])
def add_music():
    try:
        data = request.get_json()
        username = data['username']
        music_path = data['music_path']

        try:
            con = pool.get_connection()
            result = sql_tool.insert_music(con, username, music_path)
            return jsonify({"success": result})
        except Exception as e:
            print(f"Error during login: {e}")
            return jsonify({"error": "Internal Server Error"}), 500
        finally:
            print("finish login")

    except KeyError:
        return "Missing or invalid fields", 400
    except Exception as e:
        return f"Error processing request: {str(e)}", 500


def start_uwsgi():
    command = [
        "uwsgi",
        "--http", ":5000",
        "--wsgi-file", "app.py",
        "--callable", "app",
        "--gevent", "1000",
        "--http-keepalive",
        "--master",
        "--processes", "4",
        "--threads", "8",
        "--max-requests", "5000",
        "--harakiri", "60"
    ]
    subprocess.run(command)

if __name__ == "__main__":
    start_uwsgi()