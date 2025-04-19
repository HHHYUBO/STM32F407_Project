import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Conv2D, MaxPooling2D, Flatten, Dense, Dropout, BatchNormalization
from tensorflow.keras.callbacks import ModelCheckpoint, EarlyStopping, ReduceLROnPlateau
from tensorflow.keras.optimizers import Adam
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
import os
import time

# 设置随机种子以确保结果可复现
np.random.seed(42)
tf.random.set_seed(42)

def load_data(file_path):
    """
    加载合并后的特征数据
    """
    print(f"正在加载数据: {file_path}")
    
    # 读取文件，明确指定编码为UTF-8
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except UnicodeDecodeError:
        # 如果UTF-8解码失败，尝试其他编码
        with open(file_path, 'r', encoding='latin-1') as f:
            lines = f.readlines()
    
    # 跳过注释行
    data_lines = [line for line in lines if not line.startswith('#') and line.strip()]
    
    # 解析数据
    labels = []
    features_list = []
    timestamps = []
    frame_indices = []
    
    for line in data_lines:
        parts = line.strip().split(',')
        label = parts[0]
        timestamp = parts[1]
        frame_idx = int(parts[2])
        features = np.array([float(x) for x in parts[3:]])
        
        labels.append(label)
        timestamps.append(timestamp)
        frame_indices.append(frame_idx)
        features_list.append(features)
    
    # 将特征转换为numpy数组
    features_array = np.array(features_list)
    
    # 获取唯一的时间戳和标签
    unique_timestamps = np.unique(timestamps)
    unique_labels = np.unique(labels)
    
    print(f"数据加载完成，共 {len(unique_timestamps)} 个样本，{len(unique_labels)} 个类别")
    print(f"类别: {unique_labels}")
    
    # 将数据组织成样本
    samples = []
    sample_labels = []
    
    for ts in unique_timestamps:
        # 找到该时间戳的所有帧
        mask = np.array(timestamps) == ts
        ts_features = features_array[mask]
        ts_frame_indices = np.array(frame_indices)[mask]
        ts_labels = np.array(labels)[mask]
        
        # 确保帧是按顺序排列的
        sort_idx = np.argsort(ts_frame_indices)
        ts_features = ts_features[sort_idx]
        
        # 检查是否有多个标签
        unique_ts_labels = np.unique(ts_labels)
        if len(unique_ts_labels) > 1:
            print(f"警告: 时间戳 {ts} 有多个标签: {unique_ts_labels}")
            print("将为每个标签创建单独的样本")
            
            # 为每个标签创建单独的样本
            for label in unique_ts_labels:
                label_mask = ts_labels == label
                label_features = ts_features[label_mask]
                label_frame_indices = ts_frame_indices[label_mask]
                
                # 确保帧是按顺序排列的
                sort_idx = np.argsort(label_frame_indices)
                label_features = label_features[sort_idx]
                
                # 添加到样本列表
                samples.append(label_features)
                sample_labels.append(label)
        else:
            # 添加到样本列表
            samples.append(ts_features)
            sample_labels.append(ts_labels[0])
    
    return samples, sample_labels, unique_labels

def preprocess_data(samples, labels, max_frames=None, augment=True):
    """
    预处理数据，使其适合CNN输入，并进行增强的数据增强
    """
    # 编码标签
    label_encoder = LabelEncoder()
    encoded_labels = label_encoder.fit_transform(labels)
    
    # 找出最大帧数（如果未指定）
    if max_frames is None:
        max_frames = max(sample.shape[0] for sample in samples)
    
    # 获取特征维度
    feature_dim = samples[0].shape[1]
    
    # 将所有样本填充/截断到相同的帧数
    X = np.zeros((len(samples), max_frames, feature_dim))
    
    for i, sample in enumerate(samples):
        frames_to_use = min(sample.shape[0], max_frames)
        X[i, :frames_to_use, :] = sample[:frames_to_use, :]
    
    # 数据增强（如果启用）
    if augment:
        # 创建增强后的数据和标签列表
        X_augmented = [X]
        y_augmented = [encoded_labels]
        
        # 1. 添加随机噪声 - 使用多个噪声级别
        for noise_factor in [0.02, 0.05, 0.1]:
            X_noise = X.copy() + np.random.normal(0, noise_factor, X.shape)
            X_augmented.append(X_noise)
            y_augmented.append(encoded_labels)
        
        # 2. 时间拉伸/压缩（模拟语速变化）- 使用更多的缩放因子
        for scale in [0.8, 0.9, 1.1, 1.2]:
            X_scaled = []
            for sample in X:
                # 对每个样本进行时间拉伸/压缩
                frames = sample.shape[0]
                new_frames = int(frames * scale)
                if new_frames > 0:
                    # 使用简单的线性插值
                    indices = np.linspace(0, frames-1, new_frames)
                    scaled_sample = np.zeros((max_frames, feature_dim))
                    for j in range(min(new_frames, max_frames)):
                        idx = int(indices[j])
                        if idx < frames:
                            scaled_sample[j, :] = sample[idx, :]
                    X_scaled.append(scaled_sample)
            if X_scaled:
                X_scaled = np.array(X_scaled)
                if X_scaled.shape[0] > 0:
                    X_augmented.append(X_scaled)
                    y_augmented.append(encoded_labels)
        
        # 3. 特征掩码 (随机将一些特征置零，模拟特征丢失)
        mask_factor = 0.1  # 10%的特征被掩码
        X_masked = X.copy()
        mask = np.random.rand(*X_masked.shape) < mask_factor
        X_masked[mask] = 0
        X_augmented.append(X_masked)
        y_augmented.append(encoded_labels)
        
        # 合并所有增强后的数据
        X = np.vstack(X_augmented)
        encoded_labels = np.concatenate(y_augmented)
    
    # 重塑为CNN输入格式 (样本数, 帧数, 特征维度, 1)
    if len(X.shape) == 3:
        X = X.reshape(X.shape[0], X.shape[1], X.shape[2], 1)
    
    return X, encoded_labels, label_encoder

def create_ultra_lightweight_model(input_shape, num_classes):
    """
    创建改进的极简模型，完全避免使用卷积层，专为STM32F407优化
    但增加了模型容量以提高识别率
    """
    from tensorflow.keras.regularizers import l2
    
    # 计算输入特征的总维度
    total_features = input_shape[0] * input_shape[1] * input_shape[2]
    
    model = Sequential([
        # 展平层 - 直接将输入特征展平
        Flatten(input_shape=input_shape),
        
        # 第一个全连接层 - 增加神经元数量
        Dense(32, activation='relu', kernel_regularizer=l2(0.0005)),
        BatchNormalization(),
        Dropout(0.3),  # 添加dropout以减少过拟合
        
        # 第二个全连接层
        Dense(16, activation='relu', kernel_regularizer=l2(0.0005)),
        BatchNormalization(),
        
        # 输出层
        Dense(num_classes, activation='softmax')
    ])
    
    # 编译模型
    model.compile(
        optimizer=Adam(learning_rate=0.0005),  # 降低学习率
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    
    return model

def preprocess_data(samples, labels, max_frames=None, augment=True):
    """
    预处理数据，使其适合CNN输入，并进行增强的数据增强
    """
    # 编码标签
    label_encoder = LabelEncoder()
    encoded_labels = label_encoder.fit_transform(labels)
    
    # 找出最大帧数（如果未指定）
    if max_frames is None:
        max_frames = max(sample.shape[0] for sample in samples)
    
    # 获取特征维度
    feature_dim = samples[0].shape[1]
    
    # 将所有样本填充/截断到相同的帧数
    X = np.zeros((len(samples), max_frames, feature_dim))
    
    for i, sample in enumerate(samples):
        frames_to_use = min(sample.shape[0], max_frames)
        X[i, :frames_to_use, :] = sample[:frames_to_use, :]
    
    # 数据增强（如果启用）
    if augment:
        # 创建增强后的数据和标签列表
        X_augmented = [X]
        y_augmented = [encoded_labels]
        
        # 1. 添加随机噪声 - 使用多个噪声级别
        for noise_factor in [0.02, 0.05, 0.1]:
            X_noise = X.copy() + np.random.normal(0, noise_factor, X.shape)
            X_augmented.append(X_noise)
            y_augmented.append(encoded_labels)
        
        # 2. 时间拉伸/压缩（模拟语速变化）- 使用更多的缩放因子
        for scale in [0.8, 0.9, 1.1, 1.2]:
            X_scaled = []
            for sample in X:
                # 对每个样本进行时间拉伸/压缩
                frames = sample.shape[0]
                new_frames = int(frames * scale)
                if new_frames > 0:
                    # 使用简单的线性插值
                    indices = np.linspace(0, frames-1, new_frames)
                    scaled_sample = np.zeros((max_frames, feature_dim))
                    for j in range(min(new_frames, max_frames)):
                        idx = int(indices[j])
                        if idx < frames:
                            scaled_sample[j, :] = sample[idx, :]
                    X_scaled.append(scaled_sample)
            if X_scaled:
                X_scaled = np.array(X_scaled)
                if X_scaled.shape[0] > 0:
                    X_augmented.append(X_scaled)
                    y_augmented.append(encoded_labels)
        
        # 3. 特征掩码 (随机将一些特征置零，模拟特征丢失)
        mask_factor = 0.1  # 10%的特征被掩码
        X_masked = X.copy()
        mask = np.random.rand(*X_masked.shape) < mask_factor
        X_masked[mask] = 0
        X_augmented.append(X_masked)
        y_augmented.append(encoded_labels)
        
        # 合并所有增强后的数据
        X = np.vstack(X_augmented)
        encoded_labels = np.concatenate(y_augmented)
    
    # 重塑为CNN输入格式 (样本数, 帧数, 特征维度, 1)
    if len(X.shape) == 3:
        X = X.reshape(X.shape[0], X.shape[1], X.shape[2], 1)
    
    return X, encoded_labels, label_encoder

def train_model(model, X_train, y_train, X_val, y_val, batch_size=32, epochs=50, model_save_path='best_model.h5', patience=10):
    """
    训练模型，使用改进的训练策略
    """
    # 计算类权重
    from sklearn.utils.class_weight import compute_class_weight
    class_weights = compute_class_weight('balanced', classes=np.unique(y_train), y=y_train)
    class_weight_dict = {i: weight for i, weight in enumerate(class_weights)}
    
    # 创建回调函数
    callbacks = [
        ModelCheckpoint(model_save_path, save_best_only=True, monitor='val_accuracy'),
        EarlyStopping(monitor='val_accuracy', patience=patience, restore_best_weights=True),
        ReduceLROnPlateau(monitor='val_loss', factor=0.5, patience=max(3, patience//3), 
                          min_lr=0.00001, verbose=1)
    ]
    
    # 训练模型
    history = model.fit(
        X_train, y_train,
        batch_size=batch_size,
        epochs=epochs,
        validation_data=(X_val, y_val),
        callbacks=callbacks,
        class_weight=class_weight_dict,  # 添加类权重
        verbose=1
    )
    
    return history

def convert_to_c_files_simplified(model, output_dir, label_encoder, input_shape):
    """
    将简化模型转换为C头文件和源文件，适合STM32F407使用
    特别优化以减少文件大小和计算复杂度
    只生成模型权重和结构定义，不包含推理函数的实现
    """
    # 获取模型权重
    weights = model.get_weights()
    
    # 创建输出目录（如果不存在）
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 定义文件路径
    header_path = os.path.join(output_dir, "speech_model.h")
    source_path = os.path.join(output_dir, "speech_model.c")
    
    # 创建C头文件 (.h)
    with open(header_path, 'w') as f:
        f.write("/**\n")
        f.write(" * 自动生成的语音识别模型权重 (极简版本)\n")
        f.write(" * 适用于STM32F407\n")
        f.write(" */\n\n")
        f.write("#ifndef SPEECH_MODEL_H\n")
        f.write("#define SPEECH_MODEL_H\n\n")
        f.write("#include \"arm_math.h\"\n\n")
        
        # 添加模型结构信息
        f.write("// 模型结构信息\n")
        f.write(f"#define MODEL_INPUT_HEIGHT {input_shape[0]}\n")
        f.write(f"#define MODEL_INPUT_WIDTH {input_shape[1]}\n")
        f.write(f"#define MODEL_INPUT_CHANNELS {input_shape[2]}\n")
        f.write(f"#define MODEL_NUM_CLASSES {len(label_encoder.classes_)}\n\n")
        
        # 计算展平后的输入大小
        flattened_size = input_shape[0] * input_shape[1] * input_shape[2]
        f.write(f"#define MODEL_FLATTENED_SIZE {flattened_size}\n\n")
        
        # 添加层大小定义
        layer_idx = 0
        bn_idx = 0
        for i, layer in enumerate(model.layers):
            if isinstance(layer, Dense):
                weights = layer.get_weights()[0]
                weights_shape = weights.shape
                
                f.write(f"// 全连接层 {layer.name}\n")
                f.write(f"#define FC{layer_idx}_INPUT_SIZE {weights_shape[0]}\n")
                f.write(f"#define FC{layer_idx}_OUTPUT_SIZE {weights_shape[1]}\n\n")
                
                layer_idx += 1
            elif isinstance(layer, BatchNormalization):
                # 对于BatchNormalization层，使用其权重的形状来确定大小
                bn_weights = layer.get_weights()
                bn_size = len(bn_weights[0])  # gamma的长度
                
                f.write(f"// BatchNormalization层 {layer.name}\n")
                f.write(f"#define BN{bn_idx}_SIZE {bn_size}\n\n")
                
                bn_idx += 1
        
        # 添加标签映射
        f.write("// 标签映射\n")
        f.write("extern const char* LABELS[];\n\n")
        
        # 添加模型权重声明
        layer_idx = 0
        bn_idx = 0
        for i, layer in enumerate(model.layers):
            if isinstance(layer, Dense):
                f.write(f"// 全连接层 {layer.name} - 权重和偏置\n")
                f.write(f"extern const float32_t fc{layer_idx}_weights[];\n")
                f.write(f"extern const float32_t fc{layer_idx}_bias[];\n\n")
                
                layer_idx += 1
            elif isinstance(layer, BatchNormalization):
                f.write(f"// BatchNormalization层 {layer.name} - 参数\n")
                f.write(f"extern const float32_t bn{bn_idx}_gamma[];\n")
                f.write(f"extern const float32_t bn{bn_idx}_beta[];\n")
                f.write(f"extern const float32_t bn{bn_idx}_moving_mean[];\n")
                f.write(f"extern const float32_t bn{bn_idx}_moving_variance[];\n\n")
                
                bn_idx += 1
        
        # 只声明函数，不提供实现
        f.write("// 模型推理函数声明 - 实现在ASR_CNN.c中\n")
        f.write("uint8_t ASR_CNN_Recognize(float32_t input[MODEL_INPUT_HEIGHT][MODEL_INPUT_WIDTH], uint8_t *result, float32_t *confidence);\n")
        f.write("const char* ASR_CNN_GetLabel(uint8_t result);\n")
        f.write("void ASR_CNN_Init(void);\n\n")
        
        f.write("#endif // SPEECH_MODEL_H\n")
    
    # 创建C源文件 (.c) - 保持源文件部分不变
    with open(source_path, 'w') as f:
        f.write("/**\n")
        f.write(" * 自动生成的语音识别模型权重 (极简版本)\n")
        f.write(" * 适用于STM32F407\n")
        f.write(" */\n\n")
        f.write('#include "speech_model.h"\n\n')
        
        # 添加标签映射
        f.write("// 标签映射\n")
        f.write("const char* LABELS[] = {\n")
        for i, label in enumerate(label_encoder.classes_):
            f.write(f'    "{label}"')
            if i < len(label_encoder.classes_) - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")
        
        # 添加模型权重定义
        layer_idx = 0
        bn_idx = 0
        for i, layer in enumerate(model.layers):
            if isinstance(layer, Dense):
                weights = layer.get_weights()[0]
                bias = layer.get_weights()[1]
                
                # 权重
                f.write(f"// 全连接层 {layer.name} - 权重\n")
                f.write(f"const float32_t fc{layer_idx}_weights[{np.prod(weights.shape)}] = {{\n    ")
                flat_weights = weights.flatten()
                for j, w in enumerate(flat_weights):
                    f.write(f"{w:.4f}f")  # 减少小数位数
                    if j < len(flat_weights) - 1:
                        f.write(", ")
                    if (j + 1) % 8 == 0 and j < len(flat_weights) - 1:
                        f.write("\n    ")
                f.write("\n};\n\n")
                
                # 偏置
                f.write(f"// 全连接层 {layer.name} - 偏置\n")
                f.write(f"const float32_t fc{layer_idx}_bias[{len(bias)}] = {{\n    ")
                for j, b in enumerate(bias):
                    f.write(f"{b:.4f}f")  # 减少小数位数
                    if j < len(bias) - 1:
                        f.write(", ")
                    if (j + 1) % 8 == 0 and j < len(bias) - 1:
                        f.write("\n    ")
                f.write("\n};\n\n")
                
                layer_idx += 1
            elif isinstance(layer, BatchNormalization):
                gamma, beta, moving_mean, moving_variance = layer.get_weights()
                
                # Gamma
                f.write(f"// BatchNormalization层 {layer.name} - gamma\n")
                f.write(f"const float32_t bn{bn_idx}_gamma[{len(gamma)}] = {{\n    ")
                for j, g in enumerate(gamma):
                    f.write(f"{g:.4f}f")
                    if j < len(gamma) - 1:
                        f.write(", ")
                    if (j + 1) % 8 == 0 and j < len(gamma) - 1:
                        f.write("\n    ")
                f.write("\n};\n\n")
                
                # Beta
                f.write(f"// BatchNormalization层 {layer.name} - beta\n")
                f.write(f"const float32_t bn{bn_idx}_beta[{len(beta)}] = {{\n    ")
                for j, b in enumerate(beta):
                    f.write(f"{b:.4f}f")
                    if j < len(beta) - 1:
                        f.write(", ")
                    if (j + 1) % 8 == 0 and j < len(beta) - 1:
                        f.write("\n    ")
                f.write("\n};\n\n")
                
                # Moving Mean
                f.write(f"// BatchNormalization层 {layer.name} - moving_mean\n")
                f.write(f"const float32_t bn{bn_idx}_moving_mean[{len(moving_mean)}] = {{\n    ")
                for j, m in enumerate(moving_mean):
                    f.write(f"{m:.4f}f")
                    if j < len(moving_mean) - 1:
                        f.write(", ")
                    if (j + 1) % 8 == 0 and j < len(moving_mean) - 1:
                        f.write("\n    ")
                f.write("\n};\n\n")
                
                # Moving Variance
                f.write(f"// BatchNormalization层 {layer.name} - moving_variance\n")
                f.write(f"const float32_t bn{bn_idx}_moving_variance[{len(moving_variance)}] = {{\n    ")
                for j, v in enumerate(moving_variance):
                    f.write(f"{v:.4f}f")
                    if j < len(moving_variance) - 1:
                        f.write(", ")
                    if (j + 1) % 8 == 0 and j < len(moving_variance) - 1:
                        f.write("\n    ")
                f.write("\n};\n\n")
                
                bn_idx += 1
    
    print(f"简化模型已转换为C文件:\n - 头文件: {header_path} ({os.path.getsize(header_path) / 1024:.2f} KB)\n - 源文件: {source_path} ({os.path.getsize(source_path) / 1024:.2f} KB)")
    print(f"注意: 推理函数实现需在ASR_CNN.c中单独提供")

def main():
    # 设置路径
    data_dir = "d:\\22478\\Desktop\\STM32Project_Standard\\F407_project\\Speech_recognition\\mymatlab\\speech_features"
    feature_file = os.path.join(data_dir, "combined_features.txt")
    model_save_dir = "d:\\22478\\Desktop\\STM32Project_Standard\\F407_project\\Speech_recognition\\mypython\\models"
    
    # 创建模型保存目录
    if not os.path.exists(model_save_dir):
        os.makedirs(model_save_dir)
    
    # 加载数据
    samples, labels, unique_labels = load_data(feature_file)
    
    # 检查样本数量
    if len(samples) < 5:
        print(f"警告: 样本数量太少 ({len(samples)}个)，无法进行有效的训练。")
        print("请收集更多数据后再尝试训练模型。")
        print("当前样本信息:")
        for i, (sample, label) in enumerate(zip(samples, labels)):
            print(f"  样本 {i+1}: 标签={label}, 帧数={sample.shape[0]}, 特征维度={sample.shape[1]}")
        return
    
    # 预处理数据，启用增强的数据增强
    max_frames = 30  # 减少最大帧数以适应STM32F407
    X, y, label_encoder = preprocess_data(samples, labels, max_frames, augment=True)
    
    # 划分训练集、验证集和测试集
    # 如果样本数量少于10个，使用留一法交叉验证
    if len(samples) < 10:
        print("样本数量较少，使用简化的训练/测试分割...")
        # 简单地将80%用于训练，20%用于测试
        split_idx = max(1, int(len(samples) * 0.8))
        X_train = X[:split_idx]
        y_train = y[:split_idx]
        X_test = X[split_idx:]
        y_test = y[split_idx:]
        # 由于样本太少，不使用验证集，而是直接使用训练集作为验证集
        X_val = X_train
        y_val = y_train
    else:
        # 正常的训练/验证/测试分割
        X_train_val, X_test, y_train_val, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)
        X_train, X_val, y_train, y_val = train_test_split(X_train_val, y_train_val, test_size=0.2, random_state=42, stratify=y_train_val)
    
    print(f"训练集大小: {X_train.shape}")
    print(f"验证集大小: {X_val.shape}")
    print(f"测试集大小: {X_test.shape}")
    
    # 创建极简模型，不使用卷积层
    input_shape = X_train.shape[1:]
    num_classes = len(unique_labels)
    model = create_ultra_lightweight_model(input_shape, num_classes)
    
    # 打印模型摘要
    model.summary()
    
    # 训练模型
    model_save_path = os.path.join(model_save_dir, "best_model.h5")
    
    # 如果样本数量太少，减少训练轮数并增加早停的耐心值
    if len(samples) < 10:
        epochs = 50  # 增加轮数
        patience = 15
    else:
        epochs = 100  # 增加轮数
        patience = 20
    
    # 训练模型
    history = train_model(model, X_train, y_train, X_val, y_val, 
                         batch_size=min(16, len(X_train)), # 减小batch_size以适应STM32F407
                         epochs=epochs, 
                         model_save_path=model_save_path,
                         patience=patience)
    
    # 加载最佳模型
    model.load_weights(model_save_path)
    
    # 评估模型
    test_loss, test_acc = model.evaluate(X_test, y_test, verbose=1)
    print(f"测试集准确率: {test_acc:.4f}")
    
    # 将模型转换为C头文件和源文件，使用简化版本
    c_files_dir = os.path.join(model_save_dir, "c_model")
    convert_to_c_files_simplified(model, c_files_dir, label_encoder, input_shape)
    
    # 复制文件到项目目录
    bsp_dir = "d:\\22478\\Desktop\\STM32Project_Standard\\F407_project\\Speech_recognition\\bsp"
    if os.path.exists(bsp_dir):
        import shutil
        shutil.copy(os.path.join(c_files_dir, "speech_model.h"), bsp_dir)
        shutil.copy(os.path.join(c_files_dir, "speech_model.c"), bsp_dir)
        print(f"模型文件已复制到项目目录: {bsp_dir}")
    
    print("训练完成！极简模型已转换为C文件，可直接导入到STM32F407项目中。")

if __name__ == "__main__":
    start_time = time.time()
    main()
    end_time = time.time()
    print(f"总运行时间: {(end_time - start_time) / 60:.2f} 分钟")