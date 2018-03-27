function sortToDirLabels(data,dataName,divideDir,undivideDir)
disp(['START write data to dir label for movie: ' dataName]);
if ~exist(fullfile(divideDir))
    mkdir(fullfile(divideDir))
end

if ~exist(fullfile(undivideDir))
    mkdir(fullfile(undivideDir))
end

for idx=1:length(data)
    sample = data(idx);
    if 1 == sample.isDiv
        imwrite(sample.cu32,fullfile(divideDir,[dataName '_' sample.name '.png']));
    elseif 0 == sample.isDiv
        imwrite(sample.cu32,fullfile(undivideDir,[dataName '_' sample.name '.png']));
    else
        error(['incorrect label for sample' num2str(idx)])
    end
end
disp(['END write data to dir label for movie: ' dataName]);
end
