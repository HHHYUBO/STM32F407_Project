% MATLAB脚本用于处理保存的ASR数据
% 此脚本加载由串行数据接收器保存的数据，
% 绘制不同处理阶段的波形，并启用音频播放和特征可视化

clear all;
close all;

%% 加载保存的数据
% 要求用户选择数据文件
[filename, pathname] = uigetfile({'*.mat;*.txt', 'ASR数据文件 (*.mat, *.txt)'}, '选择ASR数据文件');
if isequal(filename, 0)
    disp('用户取消了文件选择');
    return;
end

fullpath = fullfile(pathname, filename);
disp(['从以下位置加载数据: ', fullpath]);

% 检查文件类型并相应地加载
[~, ~, ext] = fileparts(filename);
if strcmpi(ext, '.mat')
    % 加载MAT文件
    data = load(fullpath);
    
    % 检查我们是否有原始数据或处理过的数据
    if isfield(data, 'rawData')
        % 如果是原始数据，我们需要解析它
        dataStruct = parseRawData(data.rawData);
    elseif isfield(data, 'dataStruct')
        % 如果已经有解析好的数据结构
        dataStruct = data.dataStruct;
    else
        % 尝试使用旧格式的数据
        dataStruct = struct();
        if isfield(data, 'processed_data')
            dataStruct.processed_data = data.processed_data;
        elseif isfield(data, 'samples')
            dataStruct.processed_data = data.samples;
        end
        
        if isfield(data, 'frame_data')
            dataStruct.frame_data = data.frame_data;
        end
        
        if isfield(data, 'feature_data')
            dataStruct.feature_data = data.feature_data;
        end
        
        if isfield(data, 'mfcc')
            dataStruct.mfcc = data.mfcc;
        end
    end
else
    % 加载文本文件
    fid = fopen(fullpath, 'r');
    rawData = textscan(fid, '%s', 'Delimiter', '\n');
    fclose(fid);
    rawData = rawData{1};
    
    % 解析原始数据
    dataStruct = parseRawData(rawData);
end

%% 创建图形窗口
fs = 8000;  % 采样率（假设为8kHz）
duration = 1.0;  % 持续时间（秒）

% 创建主图形窗口
mainFig = figure('Name', ['ASR数据分析 - ', filename], 'Position', [100, 100, 1200, 800]);

% 获取处理后的信号
if isfield(dataStruct, 'processed_data') && ~isempty(dataStruct.processed_data)
    processedSignal = dataStruct.processed_data;
elseif isfield(dataStruct, 'samples') && ~isempty(dataStruct.samples)
    processedSignal = dataStruct.samples;
else
    processedSignal = [];
end

% 绘制预处理后的信号
subplot(4,1,1);
if ~isempty(processedSignal)
    % 确保我们有8000个样本（如果需要，填充或截断）
    if length(processedSignal) > 8000
        processedSignal = processedSignal(1:8000);
    elseif length(processedSignal) < 8000
        processedSignal = [processedSignal; zeros(8000 - length(processedSignal), 1)];
    end
    
    % 创建时间向量
    t_proc = (0:length(processedSignal)-1)' / fs;
    
    % 绘制预处理后的信号
    plot(t_proc, processedSignal);
    title('预处理后的信号（预加重和直流分量去除）');
    xlabel('时间 (s)');
    ylabel('幅度');
    grid on;
    
    % 添加播放预处理音频的按钮
    hPlayProcessed = uicontrol('Style', 'pushbutton', 'String', '播放预处理音频', ...
                             'Position', [1050, 740, 120, 30], ...
                             'Callback', @(src, event) playAudio(processedSignal, fs));
                             
    % 添加频谱分析按钮
    hSpectrum = uicontrol('Style', 'pushbutton', 'String', '频谱分析', ...
                         'Position', [1050, 540, 120, 30], ...
                         'Callback', @(src, event) analyzeSpectrum(processedSignal, fs));
else
    text(0.5, 0.5, '预处理数据不可用', 'HorizontalAlignment', 'center');
    title('预处理后的信号（预加重和直流分量去除）');
    axis off;
end

%% 显示加窗和分帧后的信号
subplot(4,1,2);
% 检查是否有帧数据，兼容两种可能的字段名
if isfield(dataStruct, 'frame_data') && ~isempty(dataStruct.frame_data)
    frameData = dataStruct.frame_data;
elseif isfield(dataStruct, 'frames') && ~isempty(dataStruct.frames)
    frameData = dataStruct.frames;
else
    frameData = [];
end

if ~isempty(frameData)
    hold on;
    
    colors = lines(length(frameData));
    
    for i = 1:length(frameData)
        % 兼容不同的字段名
        if isfield(frameData(i), 'windowed') && ~isempty(frameData(i).windowed)
            windowedData = frameData(i).windowed;
            if isfield(frameData(i), 'frame_idx')
                frame_idx = frameData(i).frame_idx;
            elseif isfield(frameData(i), 'index')
                frame_idx = frameData(i).index;
            else
                frame_idx = i-1; % 默认索引
            end
            
            frame_size = length(windowedData);
            
            % 从数据中计算帧重叠（如果可用）
            if isfield(dataStruct, 'frame_overlap')
                frame_overlap = dataStruct.frame_overlap;
            else
                frame_overlap = round(frame_size / 2);  % 默认50%重叠
            end
            
            % 计算帧起始样本
            frame_start_sample = frame_idx * (frame_size - frame_overlap);
            
            % 为此帧创建时间向量
            t_frame = ((0:frame_size-1) + frame_start_sample)' / fs;
            
            % 绘制加窗帧
            plot(t_frame, windowedData, 'Color', colors(i,:), 'LineWidth', 1.5);
        end
    end
    
    title('加窗和分帧后的信号');
    xlabel('时间 (s)');
    ylabel('幅度');
    grid on;
    hold off;
    
    % 添加查看详细帧数据按钮
    hFrameDetails = uicontrol('Style', 'pushbutton', 'String', '查看帧详情', ...
                             'Position', [1050, 620, 120, 30], ...
                             'Callback', @(src, event) showFrameDetails(frameData));
else
    text(0.5, 0.5, '加窗帧数据不可用', 'HorizontalAlignment', 'center');
    title('加窗和分帧后的信号');
    axis off;
end

%% 显示能量和过零率特征
subplot(4,1,3);
if isfield(dataStruct, 'feature_data') && ~isempty(dataStruct.feature_data)
    % 提取特征数据
    frame_indices = [dataStruct.feature_data.frame_idx];
    if isfield(dataStruct.feature_data(1), 'energy')
        energy_values = [dataStruct.feature_data.energy];
        has_energy = true;
    else
        has_energy = false;
    end
    
    if isfield(dataStruct.feature_data(1), 'zcr')
        zcr_values = [dataStruct.feature_data.zcr];
        has_zcr = true;
    else
        has_zcr = false;
    end
    
    % 绘制特征
    if has_energy && has_zcr
        yyaxis left;
        stem(frame_indices, energy_values, 'filled', 'LineWidth', 2);
        ylabel('能量');
        
        yyaxis right;
        stem(frame_indices, zcr_values, 'filled', 'LineWidth', 2, 'Color', 'r');
        ylabel('过零率');
        
        title('帧特征（能量和过零率）');
        xlabel('帧索引');
    elseif has_energy
        stem(frame_indices, energy_values, 'filled', 'LineWidth', 2);
        title('帧特征（能量）');
        xlabel('帧索引');
        ylabel('能量');
    elseif has_zcr
        stem(frame_indices, zcr_values, 'filled', 'LineWidth', 2, 'Color', 'r');
        title('帧特征（过零率）');
        xlabel('帧索引');
        ylabel('过零率');
    end
    
    grid on;
elseif isfield(dataStruct, 'energy') && ~isempty(dataStruct.energy) || ...
       isfield(dataStruct, 'zcr') && ~isempty(dataStruct.zcr)
    % 处理直接存储在dataStruct中的能量和过零率
    has_energy = isfield(dataStruct, 'energy') && ~isempty(dataStruct.energy);
    has_zcr = isfield(dataStruct, 'zcr') && ~isempty(dataStruct.zcr);
    
    if has_energy && has_zcr
        yyaxis left;
        stem(1:length(dataStruct.energy), dataStruct.energy, 'filled', 'LineWidth', 2);
        ylabel('能量');
        
        yyaxis right;
        stem(1:length(dataStruct.zcr), dataStruct.zcr, 'filled', 'LineWidth', 2, 'Color', 'r');
        ylabel('过零率');
        
        title('帧特征（能量和过零率）');
        xlabel('帧索引');
    elseif has_energy
        stem(1:length(dataStruct.energy), dataStruct.energy, 'filled', 'LineWidth', 2);
        title('帧特征（能量）');
        xlabel('帧索引');
        ylabel('能量');
    elseif has_zcr
        stem(1:length(dataStruct.zcr), dataStruct.zcr, 'filled', 'LineWidth', 2, 'Color', 'r');
        title('帧特征（过零率）');
        xlabel('帧索引');
        ylabel('过零率');
    end
    
    grid on;
else
    text(0.5, 0.5, '特征数据不可用', 'HorizontalAlignment', 'center');
    title('帧特征（能量和过零率）');
    axis off;
end

%% 显示MFCC特征
subplot(4,1,4);
if isfield(dataStruct, 'mfcc') && ~isempty(dataStruct.mfcc)
    % 提取MFCC数据
    try
        % 检查是否有有效的MFCC数据
        valid_mfcc = false;
        for i = 1:length(dataStruct.mfcc)
            if isfield(dataStruct.mfcc(i), 'coeffs') && any(dataStruct.mfcc(i).coeffs ~= 0)
                valid_mfcc = true;
                break;
            end
        end
        
        if valid_mfcc
            % 标准结构体数组格式
            mfcc_data = zeros(length(dataStruct.mfcc), length(dataStruct.mfcc(1).coeffs));
            for i = 1:length(dataStruct.mfcc)
                if isfield(dataStruct.mfcc(i), 'coeffs') && ~isempty(dataStruct.mfcc(i).coeffs)
                    mfcc_data(i, :) = dataStruct.mfcc(i).coeffs(:)';  % 确保是行向量
                end
            end
        else
            % 直接从原始数据重新解析MFCC
            disp('MFCC数据为空或全为0，尝试重新解析...');
            
            % 检查是否有原始数据
            if exist('data', 'var') && isfield(data, 'rawData')
                rawData = data.rawData;
                
                % 创建临时存储
                mfcc_frames = [];
                mfcc_values = {};
                current_frame = -1;
                current_coeffs = [];
                
                for i = 1:length(rawData)
                    line = rawData{i};
                    if ischar(line) || isstring(line)
                        % 检测MFCC特征帧开始
                        if contains(line, 'MFCC_FEATURE:')
                            % 如果已有数据，保存之前的帧
                            if current_frame >= 0 && ~isempty(current_coeffs)
                                mfcc_frames(end+1) = current_frame;
                                mfcc_values{end+1} = current_coeffs;
                            end
                            
                            % 开始新帧
                            parts = strsplit(line, ':');
                            current_frame = str2double(parts{2});
                            current_coeffs = [];
                        % 检测MFCC系数
                        elseif startsWith(line, 'MFCC_')
                            parts = strsplit(line, ' ');
                            if length(parts) >= 2
                                % 提取系数索引和值
                                coeff_idx_str = regexp(parts{1}, 'MFCC_(\d+)', 'tokens');
                                if ~isempty(coeff_idx_str)
                                    coeff_idx = str2double(coeff_idx_str{1}{1});
                                    coeff_val = str2double(parts{2});
                                    
                                    % 扩展系数数组
                                    while length(current_coeffs) <= coeff_idx
                                        current_coeffs(end+1) = 0;
                                    end
                                    
                                    % 保存系数值
                                    current_coeffs(coeff_idx+1) = coeff_val;
                                end
                            end
                        end
                    end
                end
                
                % 保存最后一帧
                if current_frame >= 0 && ~isempty(current_coeffs)
                    mfcc_frames(end+1) = current_frame;
                    mfcc_values{end+1} = current_coeffs;
                end
                
                % 创建MFCC矩阵
                if ~isempty(mfcc_frames)
                    % 找出最大系数数量
                    max_coeffs = 0;
                    for i = 1:length(mfcc_values)
                        max_coeffs = max(max_coeffs, length(mfcc_values{i}));
                    end
                    
                    % 创建并填充MFCC矩阵
                    mfcc_data = zeros(length(mfcc_frames), max_coeffs);
                    for i = 1:length(mfcc_frames)
                        values = mfcc_values{i};
                        mfcc_data(i, 1:length(values)) = values;
                    end
                    
                    % 更新dataStruct中的MFCC数据
                    dataStruct.mfcc = [];
                    for i = 1:length(mfcc_frames)
                        mfcc = struct('index', mfcc_frames(i), 'coeffs', mfcc_values{i}(:));
                        dataStruct.mfcc(end+1) = mfcc;
                    end
                else
                    error('无法从原始数据中提取MFCC特征');
                end
            else
                error('无原始数据可用于重新解析MFCC');
            end
        end
        
        % 绘制MFCC热图
        imagesc(mfcc_data');
        title('MFCC特征');
        xlabel('帧索引');
        ylabel('MFCC系数');
        colorbar;
        colormap('jet');
        
        % 添加查看MFCC详情按钮
        hMfccDetails = uicontrol('Style', 'pushbutton', 'String', '查看MFCC详情', ...
                                'Position', [1050, 580, 120, 30], ...
                                'Callback', @(src, event) showMfccDetails(dataStruct.mfcc));
    catch e
        disp(['MFCC显示错误: ', e.message]);
        text(0.5, 0.5, ['MFCC数据解析错误: ', e.message], 'HorizontalAlignment', 'center');
        title('MFCC特征');
        axis off;
    end
else
    text(0.5, 0.5, 'MFCC数据不可用', 'HorizontalAlignment', 'center');
    title('MFCC特征');
    axis off;
end

%% 添加控制按钮
% 导出音频按钮
if ~isempty(processedSignal)
    hExport = uicontrol('Style', 'pushbutton', 'String', '导出音频到WAV', ...
                        'Position', [1050, 700, 120, 30], ...
                        'Callback', @(src, event) exportAudio(processedSignal, fs));
end

% 添加保存图像按钮
hSaveFig = uicontrol('Style', 'pushbutton', 'String', '保存图像', ...
                     'Position', [1050, 660, 120, 30], ...
                     'Callback', @(src, event) saveFigure(mainFig, pathname, filename));

% 添加关闭窗口按钮
hClose = uicontrol('Style', 'pushbutton', 'String', '关闭窗口', ...
                   'Position', [1050, 460, 120, 30], ...
                   'Callback', @(src, event) close(mainFig));

% 添加查看详细帧数据按钮
if isfield(dataStruct, 'frame_data') && ~isempty(dataStruct.frame_data)
    hFrameDetails = uicontrol('Style', 'pushbutton', 'String', '查看帧详情', ...
                             'Position', [1050, 620, 120, 30], ...
                             'Callback', @(src, event) showFrameDetails(dataStruct.frame_data));
end

% 添加查看MFCC详情按钮
if isfield(dataStruct, 'mfcc') && ~isempty(dataStruct.mfcc)
    hMfccDetails = uicontrol('Style', 'pushbutton', 'String', '查看MFCC详情', ...
                            'Position', [1050, 580, 120, 30], ...
                            'Callback', @(src, event) showMfccDetails(dataStruct.mfcc));
end

%% 辅助函数

% 添加VAD可视化按钮到主窗口
if isfield(dataStruct, 'energy') && ~isempty(dataStruct.energy) && ...
   isfield(dataStruct, 'zcr') && ~isempty(dataStruct.zcr)
    hVAD = uicontrol('Style', 'pushbutton', 'String', '端点检测分析', ...
                     'Position', [1050, 500, 120, 30], ...
                     'Callback', @(src, event) visualizeVAD(dataStruct.energy, dataStruct.zcr));
end

% 播放音频的函数
function playAudio(signal, fs)
    % 归一化信号以防止削波
    signal = signal / max(abs(signal)) * 0.9;
    
    % 播放音频
    sound(signal, fs);
end

% 将音频导出到WAV文件的函数
function exportAudio(processed, fs)
    % 创建对话框以选择文件
    [filename, pathname] = uiputfile('*.wav', '另存为音频...');
    if isequal(filename, 0)
        return;
    end
    
    % 归一化信号
    processed = processed / max(abs(processed)) * 0.9;
    
    % 保存处理后的信号
    proc_file = fullfile(pathname, ['processed_', filename]);
    audiowrite(proc_file, processed, fs);
    
    msgbox(sprintf('音频已导出到:\n%s', proc_file), '导出完成');
end

% 保存图像的函数
function saveFigure(fig, pathname, filename)
    % 创建对话框以选择文件
    [save_filename, save_pathname] = uiputfile({'*.png', 'PNG图像 (*.png)'; ...
                                              '*.jpg', 'JPEG图像 (*.jpg)'; ...
                                              '*.fig', 'MATLAB图像 (*.fig)'}, ...
                                              '保存图像为...', ...
                                              fullfile(pathname, ['ASR_Analysis_', filename(1:end-4)]));
    if isequal(save_filename, 0)
        return;
    end
    
    % 保存图像
    saveas(fig, fullfile(save_pathname, save_filename));
    msgbox(sprintf('图像已保存到:\n%s', fullfile(save_pathname, save_filename)), '保存完成');
end

% 显示帧详情的函数
function showFrameDetails(frame_data)
    % 创建新图形
    frame_fig = figure('Name', '帧数据详情', 'Position', [150, 150, 1000, 800]);
    
    % 计算要显示的帧数量（最多显示9个帧）
    num_frames = length(frame_data);
    frames_to_show = min(9, num_frames);
    
    % 计算要显示的帧的索引
    if num_frames <= frames_to_show
        frame_indices = 1:num_frames;
    else
        % 均匀选择帧
        frame_indices = round(linspace(1, num_frames, frames_to_show));
    end
    
    for i = 1:length(frame_indices)
        frame_idx = frame_indices(i);
        frame = frame_data(frame_idx);
        
        subplot(3, 3, i);
        
        % 绘制原始帧数据和加窗后的数据
        if isfield(frame, 'original') && ~isempty(frame.original) && ...
           isfield(frame, 'windowed') && ~isempty(frame.windowed)
            plot(frame.original, 'b', 'LineWidth', 1); hold on;
            plot(frame.windowed, 'r', 'LineWidth', 1);
            
            title(['帧 #', num2str(frame.frame_idx)]);
            xlabel('样本');
            ylabel('幅度');
            legend('原始', '加窗');
            grid on;
        elseif isfield(frame, 'windowed') && ~isempty(frame.windowed)
            plot(frame.windowed, 'r', 'LineWidth', 1);
            
            title(['帧 #', num2str(frame.frame_idx)]);
            xlabel('样本');
            ylabel('幅度');
            legend('加窗');
            grid on;
        else
            text(0.5, 0.5, '帧数据不完整', 'HorizontalAlignment', 'center');
            axis off;
        end
    end
    
    sgtitle('帧数据分析', 'FontSize', 14);
end

% 显示MFCC详情的函数
function showMfccDetails(mfcc_data)
    % 创建新图形
    mfcc_fig = figure('Name', 'MFCC特征详情', 'Position', [200, 200, 1000, 800]);
    
    % 提取MFCC系数
    mfcc_matrix = zeros(length(mfcc_data), length(mfcc_data(1).coeffs));
    for i = 1:length(mfcc_data)
        mfcc_matrix(i, :) = mfcc_data(i).coeffs;
    end
    
    % 绘制MFCC热图
    subplot(2, 1, 1);
    imagesc(mfcc_matrix');
    title('MFCC特征热图');
    xlabel('帧索引');
    ylabel('MFCC系数');
    colorbar;
    colormap('jet');
    
    % 绘制选定的MFCC系数随时间变化
    subplot(2, 1, 2);
    hold on;
    
    % 选择前5个MFCC系数进行绘制
    num_coeffs = min(5, size(mfcc_matrix, 2));
    colors = lines(num_coeffs);
    
    for i = 1:num_coeffs
        plot(mfcc_matrix(:, i), 'Color', colors(i, :), 'LineWidth', 1.5);
    end
    
    title('MFCC系数随时间变化');
    xlabel('帧索引');
    ylabel('系数值');
    legend(arrayfun(@(x) ['MFCC-' num2str(x-1)], 1:num_coeffs, 'UniformOutput', false));
    grid on;
    hold off;
end

% 解析原始串行数据的函数
function dataStruct = parseRawData(rawData)
    % 初始化输出结构
    dataStruct = struct();
    dataStruct.processed_data = [];
    dataStruct.samples = [];
    dataStruct.frames = [];  % 初始化为空数组，而不是空结构体数组
    dataStruct.window = [];
    dataStruct.energy = [];
    dataStruct.zcr = [];
    dataStruct.mfcc = struct('index', {}, 'coeffs', {});    % 正确初始化为空结构体数组
    dataStruct.frame_data = struct('frame_idx', {}, 'original', {}, 'windowed', {});
    dataStruct.feature_data = struct('frame_idx', {}, 'energy', {}, 'zcr', {});
    
    % 解析状态变量
    receiving_processed = false;
    receiving_window = false;
    receiving_frame = false;
    receiving_features = false;
    current_frame_idx = 0;
    frame_size = 256; % 默认来自ASR.c
    frame_overlap = 128; % 默认来自ASR.c
    
    % 解析每一行
    for i = 1:length(rawData)
        line = rawData{i};
        
        % 确保line是字符串
        if ~ischar(line) && ~isstring(line)
            continue;
        end
        
        % 跳过空行
        if isempty(line)
            continue;
        end
        
        % 检查数据段标记
        if strcmp(line, 'DATA_BEGIN')
            continue;
        elseif strcmp(line, 'DATA_END')
            break;
        end
        
        % 处理元数据
        if startsWith(line, 'TOTAL_SAMPLES:')
            parts = split(line, ':');
            dataStruct.total_samples = str2double(parts{2});
            continue;
        elseif startsWith(line, 'FRAME_SIZE:')
            parts = split(line, ':');
            frame_size = str2double(parts{2});
            dataStruct.frame_size = frame_size;
            continue;
        elseif startsWith(line, 'FRAME_OVERLAP:')
            parts = split(line, ':');
            frame_overlap = str2double(parts{2});
            dataStruct.frame_overlap = frame_overlap;
            continue;
        elseif startsWith(line, 'TOTAL_FRAMES:')
            parts = split(line, ':');
            dataStruct.total_frames = str2double(parts{2});
            continue;
        end
        
        % 处理数据段
        if startsWith(line, 'SAMPLE')
            parts = split(line);
            if numel(parts) >= 3
                idx = str2double(parts{2}) + 1;  % MATLAB是1索引的
                val = str2double(parts{3});
                if length(dataStruct.processed_data) < idx
                    dataStruct.processed_data(idx) = val;
                    dataStruct.samples(idx) = val;
                else
                    dataStruct.processed_data(idx) = val;
                    dataStruct.samples(idx) = val;
                end
            end
            continue;
        end
        
        % 帧数据
        if startsWith(line, 'FRAME_BEGIN:')
            parts = split(line, ':');
            current_frame_idx = str2double(parts{2});
            receiving_frame = true;
            continue;
        elseif startsWith(line, 'FRAME_END:')
            receiving_frame = false;
            continue;
        end
        
        % 帧数据类型
        if startsWith(line, 'ORIGINAL_FRAME:')
            parts = split(line, ':');
            frame_idx = str2double(parts{2});
            
            % 查找此帧是否已存在于结构中
            frame_found = false;
            for j = 1:length(dataStruct.frame_data)
                if dataStruct.frame_data(j).frame_idx == frame_idx
                    frame_found = true;
                    break;
                end
            end
            
            % 如果未找到，添加新帧
            if ~frame_found
                new_frame = struct('frame_idx', frame_idx, 'original', [], 'windowed', []);
                dataStruct.frame_data(end+1) = new_frame;
            end
            continue;
        elseif startsWith(line, 'WINDOWED_FRAME:')
            parts = split(line, ':');
            frame_idx = str2double(parts{2});
            
            % 查找此帧是否已存在于结构中
            frame_found = false;
            for j = 1:length(dataStruct.frame_data)
                if dataStruct.frame_data(j).frame_idx == frame_idx
                    frame_found = true;
                    break;
                end
            end
            
            % 如果未找到，添加新帧
            if ~frame_found
                new_frame = struct('frame_idx', frame_idx, 'original', [], 'windowed', []);
                dataStruct.frame_data(end+1) = new_frame;
            end
            continue;
        end
        
        % 帧内样本数据
        if receiving_frame && startsWith(line, 'ORIG_SAMPLE')
            parts = split(line);
            if numel(parts) >= 4
                frame_idx = str2double(parts{2});
                sample_idx = str2double(parts{3}) + 1;  % MATLAB是1索引的
                val = str2double(parts{4});
                
                % 在我们的结构中查找帧
                for j = 1:length(dataStruct.frame_data)
                    if dataStruct.frame_data(j).frame_idx == frame_idx
                        % 确保数组足够大
                        if length(dataStruct.frame_data(j).original) < sample_idx
                            dataStruct.frame_data(j).original(sample_idx) = val;
                        else
                            dataStruct.frame_data(j).original(sample_idx) = val;
                        end
                        break;
                    end
                end
            end
            continue;
        elseif receiving_frame && startsWith(line, 'WIND_SAMPLE')
            parts = split(line);
            if numel(parts) >= 4
                frame_idx = str2double(parts{2});
                sample_idx = str2double(parts{3}) + 1;  % MATLAB是1索引的
                val = str2double(parts{4});
                
                % 在我们的结构中查找帧
                for j = 1:length(dataStruct.frame_data)
                    if dataStruct.frame_data(j).frame_idx == frame_idx
                        % 确保数组足够大
                        if length(dataStruct.frame_data(j).windowed) < sample_idx
                            dataStruct.frame_data(j).windowed(sample_idx) = val;
                        else
                            dataStruct.frame_data(j).windowed(sample_idx) = val;
                        end
                        break;
                    end
                end
            end
            continue;
        end
        
        % 特征数据
        if startsWith(line, 'FRAME_FEATURE:')
            parts = split(line, ':');
            feature_idx = str2double(parts{2});
            
            % 添加新特征条目
            new_feature = struct('frame_idx', feature_idx, 'energy', [], 'zcr', []);
            dataStruct.feature_data(end+1) = new_feature;
            continue;
        elseif contains(line, 'ENERGY') && ~isempty(dataStruct.feature_data)
            parts = split(line);
            if numel(parts) >= 2
                val = str2double(parts{2});
                dataStruct.feature_data(end).energy = val;
                
                % 同时更新能量数组
                feature_frame_idx = dataStruct.feature_data(end).frame_idx + 1;
                if length(dataStruct.energy) < feature_frame_idx
                    dataStruct.energy(feature_frame_idx) = val;
                else
                    dataStruct.energy(feature_frame_idx) = val;
                end
            end
            continue;
        elseif contains(line, 'ZCR') && ~isempty(dataStruct.feature_data)
            parts = split(line);
            if numel(parts) >= 2
                val = str2double(parts{2});
                dataStruct.feature_data(end).zcr = val;
                
                % 同时更新过零率数组
                feature_frame_idx = dataStruct.feature_data(end).frame_idx + 1;
                if length(dataStruct.zcr) < feature_frame_idx
                    dataStruct.zcr(feature_frame_idx) = val;
                else
                    dataStruct.zcr(feature_frame_idx) = val;
                end
            end
            continue;
        end
        
        % 解析MFCC特征数据
        if contains(line, 'MFCC_FEATURE:')
            parts = strsplit(line, ':');
            mfcc_frame_idx = str2double(parts{2});
            
            % 创建新的MFCC结构
            mfcc = struct();
            mfcc.index = mfcc_frame_idx;
            mfcc.coeffs = zeros(1, 13);  % 预分配13个MFCC系数的空间
            
            % 读取MFCC系数
            j = i + 1;
            coeff_count = 0;
            
            while j <= length(rawData) && ~contains(rawData{j}, 'MFCC_FEATURE:')
                mfcc_line = rawData{j};
                
                % 确保是字符串
                if ~ischar(mfcc_line) && ~isstring(mfcc_line)
                    j = j + 1;
                    continue;
                end
                
                % 检查是否是MFCC系数行
                if startsWith(mfcc_line, 'MFCC_')
                    parts = strsplit(mfcc_line, ' ');
                    if length(parts) >= 2
                        % 提取系数索引
                        coeff_idx_str = regexp(parts{1}, 'MFCC_(\d+)', 'tokens');
                        if ~isempty(coeff_idx_str)
                            coeff_idx = str2double(coeff_idx_str{1}{1});
                            coeff_val = str2double(parts{2});
                            
                            % 确保数组足够大
                            if coeff_idx+1 > length(mfcc.coeffs)
                                mfcc.coeffs(coeff_idx+1) = 0;
                            end
                            
                            % 保存系数值
                            mfcc.coeffs(coeff_idx+1) = coeff_val;
                            coeff_count = coeff_count + 1;
                        end
                    end
                end
                
                j = j + 1;
                
                % 如果遇到下一个特征或帧，停止
                if j <= length(rawData) && (contains(rawData{j}, 'FRAME_FEATURE:') || ...
                                        contains(rawData{j}, 'FRAME_BEGIN:') || ...
                                        contains(rawData{j}, 'DATA_END'))
                    break;
                end
            end
            
            % 只有当实际读取到系数时才添加MFCC结构
            if coeff_count > 0
                dataStruct.mfcc(end+1) = mfcc;
            end
            
            % 跳过已处理的行
            i = j - 1;
            continue;
        end

        % 解析帧数据
        
        % 解析窗函数数据
        if contains(line, 'WINDOW:')
            receiving_window = true;
            continue;
        elseif receiving_window && contains(line, 'END_WINDOW')
            receiving_window = false;
            continue;
        elseif receiving_window
            parts = split(line);
            if numel(parts) >= 2
                idx = str2double(parts{1}) + 1;  % MATLAB是1索引的
                val = str2double(parts{2});
                
                % 确保数组足够大
                if length(dataStruct.window) < idx
                    dataStruct.window(idx) = 0;
                end
                
                dataStruct.window(idx) = val;
            end
            continue;
        end
    end
    
    % 后处理：确保所有数组都是列向量
    if ~isempty(dataStruct.processed_data)
        dataStruct.processed_data = dataStruct.processed_data(:);
    end
    
    if ~isempty(dataStruct.samples)
        dataStruct.samples = dataStruct.samples(:);
    end
    
    if ~isempty(dataStruct.window)
        dataStruct.window = dataStruct.window(:);
    end
    
    if ~isempty(dataStruct.energy)
        dataStruct.energy = dataStruct.energy(:);
    end
    
    if ~isempty(dataStruct.zcr)
        dataStruct.zcr = dataStruct.zcr(:);
    end
    
    % 处理帧数据中的数组
    for i = 1:length(dataStruct.frame_data)
        if isfield(dataStruct.frame_data(i), 'original') && ~isempty(dataStruct.frame_data(i).original)
            dataStruct.frame_data(i).original = dataStruct.frame_data(i).original(:);
        end
        
        if isfield(dataStruct.frame_data(i), 'windowed') && ~isempty(dataStruct.frame_data(i).windowed)
            dataStruct.frame_data(i).windowed = dataStruct.frame_data(i).windowed(:);
        end
    end
    
    % 处理MFCC数据中的系数
    for i = 1:length(dataStruct.mfcc)
        if isfield(dataStruct.mfcc(i), 'coeffs') && ~isempty(dataStruct.mfcc(i).coeffs)
            dataStruct.mfcc(i).coeffs = dataStruct.mfcc(i).coeffs(:);
        end
    end
end

% 添加频谱分析功能
function analyzeSpectrum(signal, fs)
    % 创建新图形
    spec_fig = figure('Name', '频谱分析', 'Position', [250, 250, 1000, 800]);
    
    % 计算信号的短时傅里叶变换
    window_size = 256;
    overlap = 128;
    nfft = 512;
    
    [S, F, T] = spectrogram(signal, hamming(window_size), overlap, nfft, fs, 'yaxis');
    
    % 绘制频谱图
    subplot(2, 1, 1);
    imagesc(T, F, 10*log10(abs(S)));
    axis xy;
    title('短时傅里叶变换频谱图');
    xlabel('时间 (s)');
    ylabel('频率 (Hz)');
    colorbar;
    colormap('jet');
    
    % 绘制功率谱密度
    subplot(2, 1, 2);
    [pxx, f] = pwelch(signal, hamming(window_size), overlap, nfft, fs);
    plot(f, 10*log10(pxx));
    title('功率谱密度');
    xlabel('频率 (Hz)');
    ylabel('功率/频率 (dB/Hz)');
    grid on;
    
    % 添加频谱分析信息
    sgtitle('信号频谱分析', 'FontSize', 14);
    
    % 移除这段代码，因为在函数内部无法访问主脚本的processedSignal变量
    % if ~isempty(processedSignal)
    %     hSpectrum = uicontrol('Style', 'pushbutton', 'String', '频谱分析', ...
    %                          'Position', [1050, 540, 120, 30], ...
    %                          'Callback', @(src, event) analyzeSpectrum(processedSignal, fs));
    % end
end

% 添加语音端点检测可视化功能
function visualizeVAD(energy, zcr)
    % 创建新图形
    vad_fig = figure('Name', '语音端点检测', 'Position', [300, 300, 1000, 600]);
    
    % 绘制能量和过零率
    subplot(2, 1, 1);
    plot(energy, 'LineWidth', 1.5);
    title('帧能量');
    xlabel('帧索引');
    ylabel('能量');
    grid on;
    
    subplot(2, 1, 2);
    plot(zcr, 'r', 'LineWidth', 1.5);
    title('帧过零率');
    xlabel('帧索引');
    ylabel('过零率');
    grid on;
    
    % 尝试进行简单的端点检测
    energy_threshold = mean(energy) * 0.6;
    zcr_threshold = mean(zcr) * 1.5;
    
    % 标记可能的语音段
    speech_frames = find(energy > energy_threshold & zcr > zcr_threshold);
    
    % 如果找到语音段，在图上标记
    if ~isempty(speech_frames)
        % 找到连续的语音段
        speech_segments = findSpeechSegments(speech_frames);
        
        % 在能量图上标记语音段
        subplot(2, 1, 1);
        hold on;
        for i = 1:size(speech_segments, 1)
            start_frame = speech_segments(i, 1);
            end_frame = speech_segments(i, 2);
            
            % 绘制语音段区域
            x = [start_frame, end_frame, end_frame, start_frame];
            y = [0, 0, max(energy)*1.1, max(energy)*1.1];
            patch(x, y, 'green', 'FaceAlpha', 0.3, 'EdgeColor', 'none');
            
            % 添加标签
            text(mean([start_frame, end_frame]), max(energy)*0.9, ['语音段 #', num2str(i)], ...
                'HorizontalAlignment', 'center', 'Color', 'black', 'FontWeight', 'bold');
        end
        hold off;
        
        % 在过零率图上标记语音段
        subplot(2, 1, 2);
        hold on;
        for i = 1:size(speech_segments, 1)
            start_frame = speech_segments(i, 1);
            end_frame = speech_segments(i, 2);
            
            % 绘制语音段区域
            x = [start_frame, end_frame, end_frame, start_frame];
            y = [0, 0, max(zcr)*1.1, max(zcr)*1.1];
            patch(x, y, 'green', 'FaceAlpha', 0.3, 'EdgeColor', 'none');
        end
        hold off;
    end
    
    sgtitle('语音端点检测分析', 'FontSize', 14);
end

% 查找连续的语音段
function segments = findSpeechSegments(speech_frames)
    if isempty(speech_frames)
        segments = [];
        return;
    end
    
    % 初始化段
    segments = [];
    current_start = speech_frames(1);
    
    for i = 2:length(speech_frames)
        % 如果当前帧与前一帧不连续
        if speech_frames(i) > speech_frames(i-1) + 1
            % 结束当前段
            segments(end+1, :) = [current_start, speech_frames(i-1)];
            
            % 开始新段
            current_start = speech_frames(i);
        end
    end
    
    % 添加最后一段
    segments(end+1, :) = [current_start, speech_frames(end)];
end
