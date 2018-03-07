function sortToDirLabels(data,divideDir,undivideDir)
disp 'start write data to dir label'
if ~exist(fullfile(divideDir))
    mkdir(fullfile(divideDir))
end

if ~exist(fullfile(undivideDir))
    mkdir(fullfile(undivideDir))
end

for idx=1:length(data)
    sample = data(idx);
    if 1 == sample.isDiv
        imwrite(sample.cu32,fullfile(divideDir,[sample.name '.png']));
    elseif 0 == sample.isDiv
        imwrite(sample.cu32,fullfile(undivideDir,[sample.name '.png']));
    else
        error(['incorrect label for sample' num2str(idx)])
    end
end
disp 'end write data to dir label'
end
