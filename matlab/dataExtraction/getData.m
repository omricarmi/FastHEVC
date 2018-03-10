%%
close all;clear all;clc
%%
% files = rdir('./CNNOutput/*/*/CTU-*');

%%
path = '../../../../CNNOutput';
addpath(path);
files = getCTUsFolders(path);

%%
data = [];
for i=1:length(files)
    curData = detectAndClassify32Cu(num2str(cell2mat(files(i))));
    data = [data,curData];
end
save('dataStruct','data');
%%
load('dataStruct.mat');
sortToDirLabels(data,'div','undiv');