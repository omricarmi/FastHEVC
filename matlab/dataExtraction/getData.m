%%
close all;clear all;clc
%%
% files = rdir('./CNNOutput/*/*/CTU-*');

%%
addpath('../../CNNOutput');
files = getCTUsFolders('../../CNNOutput');

%%
detectAndClassify32Cu(num2str(cell2mat(files(1))));