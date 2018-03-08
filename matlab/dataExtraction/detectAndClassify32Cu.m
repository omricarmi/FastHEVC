function [cu32Struct ] = detectAndClassify32Cu( srcPath )
% @requires
% @effects

% C:\Users\harelnh\Desktop\winter 2018\project\FastHEVC\matlab\tmp\CTU0
% extract CTU image from srcPath


cu32Struct = struct('cu32',{},'isDiv',{},'name',{});

jsonCu00Depth = extractfield(jsondecode(fileread(fullfile(srcPath,'CU0_0.json'))),'Depth');

% no division at all
if(jsonCu00Depth == 0)
    return;
end

frameName = split(fullfile(srcPath),'\');
if isempty(frameName{end})
    frameName = frameName{end-2};
else
    frameName = frameName{end-1};
end
file = dir([srcPath '/CTU-*']);
ctuName = file.name;
fullCtuName = fullfile(srcPath,ctuName);
ctuImg = imread(fullCtuName);

cu32NameMap = {'0_0', '0_32', '32_0', '32_32'};
ctuPosMap = [0 0; 0 32; 32 0; 32 32 ];
for i=1:length(cu32NameMap)
    curJsonName = ['CU' cell2mat(cu32NameMap(i)) '.json'];
    class = 1;
    if ~isempty(dir(fullfile(srcPath,curJsonName)))
        xPos = ctuPosMap(i,1)+1;
        yPos = ctuPosMap(i,2)+1;
        %check if overflow index the ignore this part of the img
        
        curJsonData = jsondecode(fileread(fullfile(srcPath,curJsonName)));
        curStructIdx = length(cu32Struct)+1;
        
        %crop cu img
        try
            cu32Struct(curStructIdx).cu32 = ...
                ctuImg(yPos:yPos+31,xPos:xPos+31,:);
        catch ME
            switch ME.identifier
                case 'MATLAB:badsubscript'
                    warning(['ctu size:' num2str(size(ctuImg)) ' ,unsusal so we cu was skipped.']);
                    continue;
                otherwise
                    rethrow(ME) 
            end
        end
        
        %label cu
        if(curJsonData.Depth == 1)
            cu32Struct(curStructIdx).isDiv = 0;
            
        else
            cu32Struct(curStructIdx).isDiv = 1;
        end
        
        %set cu name
        cu32Struct(curStructIdx).name = cell2mat([frameName '_' ctuName(1:end-4) '_CU_' cu32NameMap(i)]);
    end
    
end

% figure;
% pos = [1,3,2,4];
% for i=1:4
%     if(pos(i) > length(cu32Struct)) continue; end
%     disp(num2str(pos(i)));
%     subplot(2,2,i);imshow(cu32Struct(pos(i)).cu32);title(num2str(i));
% end
% figure;imshow(ctuImg);
% 
% close all;
end

