function ctusFoldersList = getCTUsFolders(dataPath)
% function ctusFoldersList = getCTUsFolders(dataPath)
%   dataPath - root path of the data set without '/' at the end

 %check if bug on windows where '\' instead of '/'
tmpInfo = dir(fullfile([dataPath '/**/CTU-*']));
% ctusFoldersList = [ctusFoldersList.folder];
ctusFoldersList = {};
for i=1:length(tmpInfo)
    ctusFoldersList{i} = tmpInfo(i).folder;
end
ctusFoldersList = ctusFoldersList.';
end

