import os
import shutil

def move_files_to_named_folders(directory):
    # 创建一个字典来存储文件名对应的路径
    file_groups = {}

    for file in os.listdir(directory):
        if os.path.isfile(os.path.join(directory, file)):
            # 获取文件名（不含扩展名）和扩展名
            file_name, file_extension = os.path.splitext(file)

            # 如果文件名已经在字典中，追加到列表中
            if file_name not in file_groups:
                file_groups[file_name] = []
            file_groups[file_name].append(file)

    # 遍历字典并创建文件夹，将文件移动到对应文件夹
    for file_name, files in file_groups.items():
        folder_path = os.path.join(directory, file_name)

        # 如果文件夹不存在，则创建
        if not os.path.exists(folder_path):
            os.makedirs(folder_path)

        # 移动文件到对应文件夹
        for file in files:
            src_path = os.path.join(directory, file)
            dest_path = os.path.join(folder_path, file)
            shutil.move(src_path, dest_path)
            print(f"Moved {file} to {folder_path}")

# 示例：整理当前目录中的文件
move_files_to_named_folders("./uploads")