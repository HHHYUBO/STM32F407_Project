function mergeFeaturesToTxt(directory, outputFilename)
% mergeFeaturesToTxt - 将所有特征数据文件合并为一个txt文件
%
% 用法:
%   mergeFeaturesToTxt('speech_features', 'combined_features.txt')
%
% 输入:
%   directory - 包含特征数据文件的目录
%   outputFilename - 输出文件名
%
% 示例:
%   mergeFeaturesToTxt('speech_features', 'combined_features.txt')

    % 如果未指定目录，使用默认目录
    if nargin < 1 || isempty(directory)
        directory = 'speech_features';
    end
    
    % 如果未指定输出文件名，使用默认文件名
    if nargin < 2 || isempty(outputFilename)
        outputFilename = 'combined_features.txt';
    end
    
    % 确保目录存在
    if ~exist(directory, 'dir')
        error('目录 %s 不存在', directory);
    end
    
    % 查找所有txt文件（排除已合并的文件）
    files = dir(fullfile(directory, '*.txt'));
    
    if isempty(files)
        fprintf('未找到任何txt文件\n');
        return;
    end
    
    % 创建输出文件
    outputPath = fullfile(directory, outputFilename);
    fid = fopen(outputPath, 'w');
    
    % 写入文件头
    fprintf(fid, '# 合并特征数据文件\n');
    fprintf(fid, '# 创建时间: %s\n', datestr(now));
    fprintf(fid, '# 文件格式: 标签,时间戳,帧索引,特征值...\n');
    fprintf(fid, '# 合并文件数: %d\n\n', length(files));
    
    % 记录处理的样本数
    sampleCount = 0;
    
    % 修改这部分代码，确保时间戳唯一
    % 遍历所有文件
    for i = 1:length(files)
        filePath = fullfile(directory, files(i).name);
        
        % 跳过合并文件本身
        if strcmp(files(i).name, outputFilename)
            continue;
        end
        
        fprintf('处理文件 %d/%d: %s\n', i, length(files), files(i).name);
        
        % 读取文件
        fidIn = fopen(filePath, 'r');
        
        % 读取头信息
        headerLine1 = fgetl(fidIn);
        headerLine2 = fgetl(fidIn);
        headerLine3 = fgetl(fidIn);
        headerLine4 = fgetl(fidIn);
        headerLine5 = fgetl(fidIn);
        
        % 提取标签
        label = strtrim(extractAfter(headerLine1, '# 语音指令:'));
        
        % 提取帧数和特征维度
        numFrames = str2double(extractAfter(headerLine3, '# 帧数:'));
        featureDim = str2double(extractAfter(headerLine4, '# 特征维度:'));
        
        % 提取时间戳 - 修改为使用文件名和索引确保唯一性
        nameParts = split(files(i).name, '_');
        if length(nameParts) >= 2
            % 使用文件名和索引i确保时间戳唯一
            timestamp = sprintf('%s_%d', nameParts{2}, i);
        else
            % 使用当前时间和索引i确保时间戳唯一
            timestamp = sprintf('%s_%d', datestr(now, 'yyyymmdd_HHMMSS'), i);
        end
        
        % 读取特征数据并写入合并文件
        for j = 1:numFrames
            line = fgetl(fidIn);
            if ischar(line)
                values = strtrim(line);
                % 写入格式: 标签,时间戳,帧索引,特征值...
                fprintf(fid, '%s,%s,%d,%s\n', label, timestamp, j, values);
            end
        end
        
        fclose(fidIn);
        sampleCount = sampleCount + 1;
    end
    
    fclose(fid);
    fprintf('特征数据合并完成，共处理 %d 个样本\n', sampleCount);
    fprintf('合并文件已保存为: %s\n', outputPath);
    
    % 显示合并文件的前几行
    try
        fidPreview = fopen(outputPath, 'r');
        fprintf('\n合并文件预览:\n');
        for i = 1:min(10, sampleCount * numFrames + 5)
            line = fgetl(fidPreview);
            if ischar(line)
                fprintf('%s\n', line);
            else
                break;
            end
        end
        fclose(fidPreview);
        fprintf('...\n');
    catch
        fprintf('无法预览合并文件\n');
    end
end