% 生成梅尔滤波器系数并导出为C语言数组
clear all;

% 参数设置
nfft = 512;           % FFT点数
num_filters = 26;     % 梅尔滤波器数量
sample_rate = 8000;   % 采样率
low_freq = 0;         % 最低频率
high_freq = sample_rate/2; % 最高频率

% 将频率转换为梅尔频率
low_freq_mel = 2595 * log10(1 + low_freq/700);
high_freq_mel = 2595 * log10(1 + high_freq/700);

% 在梅尔刻度上均匀分布滤波器
mel_points = linspace(low_freq_mel, high_freq_mel, num_filters+2);
hz_points = 700 * (10.^(mel_points/2595) - 1);

% 将Hz转换为FFT bin索引
bin_indices = floor((nfft+1) * hz_points / sample_rate);

% 创建滤波器组
fbank = zeros(num_filters, nfft/2+1);
for m = 1:num_filters
    for k = 1:nfft/2+1
        if k >= bin_indices(m) && k <= bin_indices(m+1)
            % 上升部分
            fbank(m, k) = (k - bin_indices(m)) / (bin_indices(m+1) - bin_indices(m));
        elseif k >= bin_indices(m+1) && k <= bin_indices(m+2)
            % 下降部分
            fbank(m, k) = (bin_indices(m+2) - k) / (bin_indices(m+2) - bin_indices(m+1));
        end
    end
end

% 计算每个滤波器的非零系数
sparse_indices = cell(num_filters, 1);
sparse_values = cell(num_filters, 1);
max_nonzero = 0;

for m = 1:num_filters
    nonzero_idx = find(fbank(m, :) > 0);
    sparse_indices{m} = nonzero_idx - 1; % C语言索引从0开始
    sparse_values{m} = fbank(m, nonzero_idx);
    
    % 记录最大非零系数数量
    max_nonzero = max(max_nonzero, length(nonzero_idx));
    
    % 输出每个滤波器的非零系数数量
    fprintf('滤波器 %d: %d 个非零系数\n', m, length(nonzero_idx));
end

% 导出为C语言数组 - 稀疏表示
fid = fopen('../bsp/mel_filters_sparse.h', 'w');
fprintf(fid, '// 自动生成的梅尔滤波器系数 (稀疏表示)\n');
fprintf(fid, '// 参数: nfft=%d, num_filters=%d, sample_rate=%d\n\n', nfft, num_filters, sample_rate);
fprintf(fid, '#ifndef MEL_FILTERS_SPARSE_H\n');
fprintf(fid, '#define MEL_FILTERS_SPARSE_H\n\n');
fprintf(fid, '#include "arm_math.h"\n\n');
fprintf(fid, '#define NUM_FILTERS %d\n', num_filters);
fprintf(fid, '#define NFFT_HALF %d\n', nfft/2+1);
fprintf(fid, '#define MAX_NONZERO %d\n\n', max_nonzero);

% 输出非零系数的索引
fprintf(fid, '// 每个滤波器的非零系数索引\n');
fprintf(fid, 'const uint16_t mel_indices[NUM_FILTERS][MAX_NONZERO] = {\n');
for m = 1:num_filters
    fprintf(fid, '    {');
    indices = sparse_indices{m};
    for i = 1:length(indices)
        fprintf(fid, '%d', indices(i));
        if i < length(indices)
            fprintf(fid, ', ');
        end
        % 每行16个数，提高可读性
        if mod(i, 16) == 0 && i < length(indices)
            fprintf(fid, '\n     ');
        end
    end
    
    % 如果当前滤波器的非零系数少于最大值，用0填充
    if length(indices) < max_nonzero
        if length(indices) > 0
            fprintf(fid, ', ');
        end
        for i = length(indices)+1:max_nonzero
            fprintf(fid, '0');
            if i < max_nonzero
                fprintf(fid, ', ');
            end
            % 每行16个数，提高可读性
            if mod(i-length(indices), 16) == 0 && i < max_nonzero
                fprintf(fid, '\n     ');
            end
        end
    end
    
    if m < num_filters
        fprintf(fid, '},\n');
    else
        fprintf(fid, '}\n');
    end
end
fprintf(fid, '};\n\n');

% 输出非零系数的值
fprintf(fid, '// 每个滤波器的非零系数值\n');
fprintf(fid, 'const float32_t mel_values[NUM_FILTERS][MAX_NONZERO] = {\n');
for m = 1:num_filters
    fprintf(fid, '    {');
    values = sparse_values{m};
    for i = 1:length(values)
        fprintf(fid, '%.6ff', values(i));
        if i < length(values)
            fprintf(fid, ', ');
        end
        % 每行8个数，提高可读性
        if mod(i, 8) == 0 && i < length(values)
            fprintf(fid, '\n     ');
        end
    end
    
    % 如果当前滤波器的非零系数少于最大值，用0填充
    if length(values) < max_nonzero
        if length(values) > 0
            fprintf(fid, ', ');
        end
        for i = length(values)+1:max_nonzero
            fprintf(fid, '0.0f');
            if i < max_nonzero
                fprintf(fid, ', ');
            end
            % 每行8个数，提高可读性
            if mod(i-length(values), 8) == 0 && i < max_nonzero
                fprintf(fid, '\n     ');
            end
        end
    end
    
    if m < num_filters
        fprintf(fid, '},\n');
    else
        fprintf(fid, '}\n');
    end
end
fprintf(fid, '};\n\n');

% 输出每个滤波器的非零系数数量
fprintf(fid, '// 每个滤波器的非零系数数量\n');
fprintf(fid, 'const uint16_t mel_nonzero_count[NUM_FILTERS] = {\n    ');
for m = 1:num_filters
    fprintf(fid, '%d', length(sparse_indices{m}));
    if m < num_filters
        fprintf(fid, ', ');
    end
    % 每行16个数，提高可读性
    if mod(m, 16) == 0 && m < num_filters
        fprintf(fid, '\n    ');
    end
end
fprintf(fid, '\n};\n\n');

fprintf(fid, '#endif // MEL_FILTERS_SPARSE_H\n');
fclose(fid);

% 同时保留原始的完整系数导出
% 导出为C语言数组
fid = fopen('mel_filters.h', 'w');
fprintf(fid, '// 自动生成的梅尔滤波器系数\n');
fprintf(fid, '// 参数: nfft=%d, num_filters=%d, sample_rate=%d\n\n', nfft, num_filters, sample_rate);
fprintf(fid, '#ifndef MEL_FILTERS_H\n');
fprintf(fid, '#define MEL_FILTERS_H\n\n');
fprintf(fid, '#include "arm_math.h"\n\n');
fprintf(fid, '#define NUM_FILTERS %d\n', num_filters);
fprintf(fid, '#define NFFT_HALF %d\n\n', nfft/2+1);

% 输出滤波器系数
fprintf(fid, 'const float32_t mel_fbank[NUM_FILTERS][NFFT_HALF] = {\n');
for m = 1:num_filters
    fprintf(fid, '    {');
    for k = 1:nfft/2+1
        fprintf(fid, '%.6ff', fbank(m, k));
        if k < nfft/2+1
            fprintf(fid, ', ');
        end
        % 每行16个数，提高可读性
        if mod(k, 16) == 0 && k < nfft/2+1
            fprintf(fid, '\n     ');
        end
    end
    if m < num_filters
        fprintf(fid, '},\n');
    else
        fprintf(fid, '}\n');
    end
end
fprintf(fid, '};\n\n');
fprintf(fid, '#endif // MEL_FILTERS_H\n');
fclose(fid);

disp('梅尔滤波器系数已生成并保存到:');
disp('1. mel_filters.h (完整表示)');
disp('2. mel_filters_sparse.h (稀疏表示)');

% 计算内存占用
full_size = num_filters * (nfft/2+1) * 4; % 每个float32占4字节
sparse_size = (num_filters * max_nonzero * 4) + (num_filters * max_nonzero * 2) + (num_filters * 2); % 值(4字节) + 索引(2字节) + 计数(2字节)
fprintf('完整表示内存占用: %.2f KB\n', full_size/1024);
fprintf('稀疏表示内存占用: %.2f KB\n', sparse_size/1024);
fprintf('内存节省: %.2f%%\n', (1 - sparse_size/full_size) * 100);

% 可视化滤波器组
figure;
for m = 1:num_filters
    plot(1:nfft/2+1, fbank(m, :));
    hold on;
end
title('梅尔滤波器组');
xlabel('FFT Bin');
ylabel('幅度');