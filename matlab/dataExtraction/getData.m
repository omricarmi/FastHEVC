""%%
close all;clear all;clc
%%
% files = rdir('./CNNOutput/*/*/CTU-*');

%%
curVideo = 'cheers';
path = ['/home/omriharel/Desktop/dataOutput/CNNDataset_' curVideo];
addpath(path);
files = getCTUsFolders(path);

%%
isOffline = 1;
if(isOffline)
    data = [];
    for i=1:length(files)
        curData = detectAndClassify32Cu(num2str(cell2mat(files(i))));
        data = [data,curData];
    end
    save('dataStruct','data');
end

%%
load('dataStruct.mat');
dividedPath = ['/home/omriharel/Desktop/dataOutput/dividedData/' curVideo '/'];
sortToDirLabels(data,curVideo,[dividedPath 'div'], [dividedPath 'undiv']);