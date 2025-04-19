% 简化版CNN语音指令数据采集工具
% 仅接收特征数据并保存为txt文件

clear;
clc;

% 配置串口
serialPort = 'COM12';  % 根据实际情况修改COM口
baudRate = 115200;

% 创建保存目录
saveDir = 'speech_features';
if ~exist(saveDir, 'dir')
    mkdir(saveDir);
end

% 创建一个图形窗口用于显示数据和处理按键
fig = figure('Name', '语音数据采集工具', 'KeyPressFcn', @keyPressCallback);
set(fig, 'Position', [100, 100, 800, 600]);

% 创建一个文本区域显示状态
statusText = uicontrol('Style', 'text', 'Position', [20, 20, 760, 30], ...
                       'String', '等待STM32发送数据...按P键播放最后一次采集的语音');

% 全局变量
lastWaveform = [];  % 存储最后一次采集的波形数据
sampleRate = 8000;  % 采样率
sampleCount = 0;    % 样本计数
playKeyPressed = false; % 播放按键标志

% 打开串口
try
    s = serialport(serialPort, baudRate);
    s.Timeout = 30;  % 设置超时时间为30秒
    fprintf('串口 %s 已打开\n', serialPort);
catch e
    fprintf('错误: 无法打开串口 %s: %s\n', serialPort, e.message);
    return;
end

% 主循环
running = true;

while running
    % 更新状态文本
    set(statusText, 'String', '等待STM32发送数据...按P键播放最后一次采集的语音');
    drawnow;
    
    % 检查是否按下了播放键
    if playKeyPressed && ~isempty(lastWaveform)
        playKeyPressed = false;
        
        % 播放最后一次采集的波形
        fprintf('播放最后一次采集的语音...\n');
        
        % 显示波形
        subplot(2,1,1);
        plot(lastWaveform);
        title('正在播放的语音波形');
        xlabel('样本');
        ylabel('幅度');
        drawnow;
        
        % 播放音频
        sound(lastWaveform, sampleRate);
        continue;
    end
    
    % 检查串口是否有数据可读
    if s.NumBytesAvailable > 0
        % 读取一行数据
        line = readline(s);
        
        % 检查是否是数据开始标记
        if contains(line, 'CNN_DATA_BEGIN')
            fprintf('开始接收数据...\n');
            set(statusText, 'String', '正在接收数据...');
            drawnow;
            
            % 解析语音段信息
            line = readline(s);
            speech_start = str2double(extractAfter(line, 'SPEECH_START:'));
            
            line = readline(s);
            speech_end = str2double(extractAfter(line, 'SPEECH_END:'));
            
            line = readline(s);
            valid_frames = str2double(extractAfter(line, 'VALID_FRAMES:'));
            
            line = readline(s);
            feature_dim = str2double(extractAfter(line, 'FEATURE_DIM:'));
            
            % 准备接收特征数据
            features = zeros(valid_frames, feature_dim);
            
            % 读取特征数据开始标记
            line = readline(s);
            if ~contains(line, 'FEATURE_DATA_BEGIN')
                fprintf('错误：未找到特征数据开始标记\n');
                continue;
            end
            
            % 读取特征数据
            for i = 1:valid_frames
                frame_line = readline(s);  % 读取帧号
                data_line = readline(s);   % 读取特征数据
                
                % 解析特征数据
                values = str2double(split(data_line, ','));
                if length(values) == feature_dim
                    features(i, :) = values;
                else
                    fprintf('警告：帧 %d 的特征维度不匹配\n', i);
                end
            end
            
            % 读取特征数据结束标记
            line = readline(s);
            if ~contains(line, 'FEATURE_DATA_END')
                fprintf('错误：未找到特征数据结束标记\n');
                continue;
            end
            
            % 读取波形数据
            line = readline(s);
            if ~contains(line, 'RAW_WAVEFORM_BEGIN')
                fprintf('错误：未找到波形数据开始标记\n');
                continue;
            end
            
            line = readline(s);
            num_samples = str2double(extractAfter(line, 'SAMPLES:'));
            
            line = readline(s);
            waveform_values = str2double(split(line, ','));
            
            % 保存波形数据用于播放
            lastWaveform = waveform_values;
            
            line = readline(s);
            if ~contains(line, 'RAW_WAVEFORM_END')
                fprintf('错误：未找到波形数据结束标记\n');
                continue;
            end
            
            % 等待数据结束标记
            line = readline(s);
            if ~contains(line, 'CNN_DATA_END')
                fprintf('错误：未找到数据结束标记\n');
                continue;
            end
            
            % 显示波形和特征
            subplot(2,1,1);
            plot(waveform_values);
            title('语音波形');
            xlabel('样本');
            ylabel('幅度');
            
            subplot(2,1,2);
            imagesc(features');
            title('MFCC特征');
            colorbar;
            xlabel('帧');
            ylabel('特征维度');
            drawnow;
            
            % 请求用户输入标签
            set(statusText, 'String', '请在命令窗口输入当前语音指令的标签');
            drawnow;
            label = input('请输入当前语音指令的标签: ', 's');
            
            if strcmpi(label, 'exit') || strcmpi(label, 'quit')
                running = false;
                continue;
            end
            
            % 生成文件名
            sampleCount = sampleCount + 1;
            timestamp = datestr(now, 'yyyymmdd_HHMMSS');
            filename = fullfile(saveDir, sprintf('%s_%s_%03d.txt', label, timestamp, sampleCount));
            
            % 保存特征到txt文件
            fid = fopen(filename, 'w');
            fprintf(fid, '# 语音指令: %s\n', label);
            fprintf(fid, '# 采集时间: %s\n', datestr(now));
            fprintf(fid, '# 帧数: %d\n', valid_frames);
            fprintf(fid, '# 特征维度: %d\n', feature_dim);
            fprintf(fid, '# 格式: 每行一帧，每帧%d个特征值，用逗号分隔\n', feature_dim);
            
            for i = 1:size(features, 1)
                for j = 1:size(features, 2)
                    fprintf(fid, '%.6f', features(i, j));
                    if j < size(features, 2)
                        fprintf(fid, ',');
                    end
                end
                fprintf(fid, '\n');
            end
            fclose(fid);
            
            % 同时保存波形数据
            waveFilename = fullfile(saveDir, sprintf('%s_%s_%03d_wave.mat', label, timestamp, sampleCount));
            save(waveFilename, 'waveform_values', 'sampleRate');
            
            fprintf('特征数据已保存到文件: %s\n', filename);
            fprintf('波形数据已保存到文件: %s\n', waveFilename);
            
            % 询问是否播放刚采集的语音
            set(statusText, 'String', '请在命令窗口选择是否播放刚采集的语音');
            drawnow;
            playResponse = input('是否播放刚采集的语音？(y/n): ', 's');
            if strcmpi(playResponse, 'y')
                % 播放音频
                sound(waveform_values, sampleRate);
                fprintf('正在播放语音...\n');
            end
            
            % 询问是否继续
            set(statusText, 'String', '请在命令窗口选择是否继续采集数据');
            drawnow;
            response = input('继续采集数据？(y/n): ', 's');
            if strcmpi(response, 'n')
                running = false;
            end
        end
    else
        % 如果没有数据，短暂暂停以减少CPU使用
        pause(0.1);
    end
end

% 关闭串口
clear s;
fprintf('数据采集完成，共采集 %d 个样本\n', sampleCount);
fprintf('所有特征数据已保存在 %s 目录下\n', saveDir);


% 按键回调函数
function keyPressCallback(src, event)
    % 获取全局变量
    global playKeyPressed lastWaveform;
    
    % 检查按下的键
    if strcmp(event.Key, 'p')
        playKeyPressed = true;
        if isempty(lastWaveform)
            fprintf('没有可播放的语音数据\n');
        end
    end
end
