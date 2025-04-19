import numpy as np
import os

def check_combined_data(file_path):
    """
    检查合并后的特征数据文件
    """
    print(f"正在分析数据文件: {file_path}")
    
    # 读取文件
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except UnicodeDecodeError:
        with open(file_path, 'r', encoding='latin-1') as f:
            lines = f.readlines()
    
    # 跳过注释行
    data_lines = [line for line in lines if not line.startswith('#') and line.strip()]
    
    # 解析数据
    labels = []
    timestamps = []
    
    for line in data_lines:
        parts = line.strip().split(',')
        label = parts[0]
        timestamp = parts[1]
        
        labels.append(label)
        timestamps.append(timestamp)
    
    # 分析数据
    unique_timestamps = np.unique(timestamps)
    unique_labels = np.unique(labels)
    
    print(f"总行数: {len(data_lines)}")
    print(f"唯一时间戳数量: {len(unique_timestamps)}")
    print(f"唯一标签数量: {len(unique_labels)}")
    print(f"标签: {unique_labels}")
    
    # 分析每个标签的样本数量
    label_counts = {}
    for label in unique_labels:
        # 找出该标签的所有时间戳
        label_timestamps = []
        for i, l in enumerate(labels):
            if l == label:
                label_timestamps.append(timestamps[i])
        
        # 计算该标签的唯一时间戳数量
        unique_label_timestamps = np.unique(label_timestamps)
        label_counts[label] = len(unique_label_timestamps)
    
    print("\n每个标签的样本数量:")
    for label, count in label_counts.items():
        print(f"  {label}: {count} 个样本")
    
    # 检查时间戳是否有多个标签
    timestamp_labels = {}
    for i, ts in enumerate(timestamps):
        if ts not in timestamp_labels:
            timestamp_labels[ts] = set()
        timestamp_labels[ts].add(labels[i])
    
    multi_label_timestamps = [ts for ts, lbls in timestamp_labels.items() if len(lbls) > 1]
    
    if multi_label_timestamps:
        print("\n警告: 以下时间戳有多个标签:")
        for ts in multi_label_timestamps:
            print(f"  时间戳 {ts} 的标签: {timestamp_labels[ts]}")

if __name__ == "__main__":
    data_dir = "../mymatlab/speech_features"
    feature_file = os.path.join(data_dir, "combined_features.txt")
    
    if os.path.exists(feature_file):
        check_combined_data(feature_file)
    else:
        print(f"错误: 文件 {feature_file} 不存在")