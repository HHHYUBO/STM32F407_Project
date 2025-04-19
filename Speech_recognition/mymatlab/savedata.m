% MATLAB code to receive and save raw ASR data from microcontroller over serial

%% Setup serial connection
clear all;
close all;

% Configure serial port - adjust COM port as needed
serialPort = 'COM12';  % Change to your COM port
baudRate = 115200;    % Set to match your microcontroller's baud rate

% Create serial object
s = serialport(serialPort, baudRate);
s.Timeout = 60;  % Timeout in seconds

% Prepare data container
rawData = {};
lineCount = 0;

disp('Starting serial data reception...');
disp('Waiting for data from microcontroller...');

% Main reception loop
try
    % Wait for data to begin
    while true
        line = readline(s);
        line = strip(line);
        
        % Skip empty lines
        if isempty(line)
            continue;
        end
        
        % Save the line
        lineCount = lineCount + 1;
        rawData{lineCount} = line;
        
        % Display feedback every 100 lines
        if mod(lineCount, 100) == 0
            fprintf('Received %d lines\n', lineCount);
        end
        
        % Check for data end marker
        if strcmp(line, 'DATA_END')
            disp('Data transmission completed');
            break;  % End reception
        end
    end
    
    % Clear serial port
    clear s;
    
    % 保存原始数据到文本文件
    save_filename = ['ASR_RawData_', datestr(now, 'yyyymmdd_HHMMSS'), '.txt'];
    fileID = fopen(save_filename, 'w');
    for i = 1:length(rawData)
        fprintf(fileID, '%s\n', rawData{i});
    end
    fclose(fileID);
    disp(['原始数据已保存到文本文件: ', save_filename]);
    
    % 同时保存为MAT文件以便于MATLAB处理
    mat_filename = ['ASR_RawData_', datestr(now, 'yyyymmdd_HHMMSS'), '.mat'];
    save(mat_filename, 'rawData');
    disp(['原始数据也已保存到MAT文件: ', mat_filename]);
    
    % 询问用户是否要解析数据
    parse_data = input('是否要解析接收到的数据? (y/n): ', 's');
    if strcmpi(parse_data, 'y')
        try
            dataStruct = parseRawData(rawData);
            
            % 保存解析后的数据
            parsed_filename = ['ASR_ParsedData_', datestr(now, 'yyyymmdd_HHMMSS'), '.mat'];
            save(parsed_filename, 'dataStruct');
            disp(['解析后的数据已保存到: ', parsed_filename]);
        catch e
            disp(['解析数据时出错: ', e.message]);
            disp('仍然保存了原始数据');
        end
    end
    
catch e
    disp(['Error: ', e.message]);
    % Clear serial port in case of error
    if exist('s', 'var')
        clear s;
    end
end

%% 解析原始数据的函数
function dataStruct = parseRawData(rawData)
    % 初始化数据结构
    dataStruct = struct();
    dataStruct.samples = [];
    dataStruct.frames = [];  % 初始化为空数组，而不是空结构体数组
    dataStruct.window = [];
    dataStruct.energy = [];
    dataStruct.zcr = [];
    dataStruct.mfcc = [];    % 初始化为空数组，而不是空结构体数组
    
    % 解析参数
    i = 1;
    while i <= length(rawData)
        line = rawData{i};
        
        % 解析总样本数
        if contains(line, 'TOTAL_SAMPLES:')
            parts = strsplit(line, ':');
            dataStruct.totalSamples = str2double(parts{2});
        end
        
        % 解析样本数据
        if contains(line, 'SAMPLE ')
            parts = strsplit(line, ' ');
            if length(parts) >= 3
                idx = str2double(parts{2}) + 1; % MATLAB索引从1开始
                val = str2double(parts{3});
                
                % 确保数组足够大
                if length(dataStruct.samples) < idx
                    dataStruct.samples(idx) = 0;
                end
                
                dataStruct.samples(idx) = val;
            end
        end
        
        % 解析窗函数数据
        if contains(line, 'WINDOW ')
            parts = strsplit(line, ' ');
            if length(parts) >= 3
                idx = str2double(parts{2}) + 1; % MATLAB索引从1开始
                val = str2double(parts{3});
                
                % 确保数组足够大
                if length(dataStruct.window) < idx
                    dataStruct.window(idx) = 0;
                end
                
                dataStruct.window(idx) = val;
            end
        end
        
        % 解析帧数据
        if contains(line, 'FRAME_BEGIN:')
            parts = strsplit(line, ':');
            frame_idx = str2double(parts{2});  % 保持原始索引
            
            % 创建新帧结构
            frame = struct();
            frame.index = frame_idx;
            frame.original = [];
            frame.windowed = [];
            
            % 读取帧数据
            j = i + 1;
            while j <= length(rawData) && ~contains(rawData{j}, 'FRAME_END:')
                frame_line = rawData{j};
                
                % 解析原始帧数据
                if contains(frame_line, 'ORIG_SAMPLE')
                    parts = strsplit(frame_line, ' ');
                    if length(parts) >= 4
                        sample_idx = str2double(parts{3}) + 1;
                        sample_val = str2double(parts{4});
                        
                        % 确保数组足够大
                        if length(frame.original) < sample_idx
                            frame.original(sample_idx) = 0;
                        end
                        
                        frame.original(sample_idx) = sample_val;
                    end
                end
                
                % 解析加窗后的帧数据
                if contains(frame_line, 'WIND_SAMPLE')
                    parts = strsplit(frame_line, ' ');
                    if length(parts) >= 4
                        sample_idx = str2double(parts{3}) + 1;
                        sample_val = str2double(parts{4});
                        
                        % 确保数组足够大
                        if length(frame.windowed) < sample_idx
                            frame.windowed(sample_idx) = 0;
                        end
                        
                        frame.windowed(sample_idx) = sample_val;
                    end
                end
                
                j = j + 1;
            end
            
            % 保存帧数据 - 使用数组追加方式
            dataStruct.frames = [dataStruct.frames; frame];
            i = j; % 跳过已处理的行
            continue;
        end
        
        % 解析特征数据
        if contains(line, 'FRAME_FEATURE:')
            parts = strsplit(line, ':');
            feature_frame_idx = str2double(parts{2}) + 1;
            
            % 读取特征
            j = i + 1;
            while j <= length(rawData) && (contains(rawData{j}, 'ENERGY') || contains(rawData{j}, 'ZCR'))
                feature_line = rawData{j};
                
                % 解析能量特征
                if contains(feature_line, 'ENERGY')
                    parts = strsplit(feature_line, ' ');
                    if length(parts) >= 2
                        energy_val = str2double(parts{2});
                        % 确保数组足够大
                        if length(dataStruct.energy) < feature_frame_idx
                            dataStruct.energy(feature_frame_idx) = 0;
                        end
                        dataStruct.energy(feature_frame_idx) = energy_val;
                    end
                end
                
                % 解析过零率特征
                if contains(feature_line, 'ZCR')
                    parts = strsplit(feature_line, ' ');
                    if length(parts) >= 2
                        zcr_val = str2double(parts{2});
                        % 确保数组足够大
                        if length(dataStruct.zcr) < feature_frame_idx
                            dataStruct.zcr(feature_frame_idx) = 0;
                        end
                        dataStruct.zcr(feature_frame_idx) = zcr_val;
                    end
                end
                
                j = j + 1;
            end
            
            i = j - 1; % 回退一行，因为下面会增加i
        end
        
        % 解析MFCC特征数据
        if contains(line, 'MFCC_FEATURE:')
            parts = strsplit(line, ':');
            mfcc_frame_idx = str2double(parts{2});
            
            % 创建新的MFCC结构
            mfcc = struct();
            mfcc.index = mfcc_frame_idx;
            mfcc.coeffs = [];
            
            % 读取MFCC系数
            j = i + 1;
            while j <= length(rawData) && contains(rawData{j}, 'MFCC_')
                mfcc_line = rawData{j};
                parts = strsplit(mfcc_line, ' ');
                if length(parts) >= 2
                    coeff_part = parts{1};
                    % 提取MFCC_后面的数字
                    coeff_str = extractBetween(coeff_part, 'MFCC_', ' ');
                    if ~isempty(coeff_str)
                        coeff_idx = str2double(coeff_str{1}) + 1;
                        coeff_val = str2double(parts{2});
                        
                        % 确保数组足够大
                        if length(mfcc.coeffs) < coeff_idx
                            mfcc.coeffs(coeff_idx) = 0;
                        end
                        
                        mfcc.coeffs(coeff_idx) = coeff_val;
                    end
                end
                
                j = j + 1;
            end
            
            % 保存MFCC数据 - 使用数组追加方式
            dataStruct.mfcc = [dataStruct.mfcc; mfcc];
            i = j - 1; % 回退一行，因为下面会增加i
        end
        
        i = i + 1;
    end
    
    % 检查是否有MFCC数据
    if isempty(dataStruct.mfcc)
        disp('警告: 未找到MFCC特征数据');
    else
        disp(['找到 ', num2str(length(dataStruct.mfcc)), ' 帧MFCC特征数据']);
    end
end