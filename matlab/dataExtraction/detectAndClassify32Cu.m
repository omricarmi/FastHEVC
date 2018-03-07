function [cu32Struct ] = detectAndClassify32Cu( srcPath )
% @requires
% @effects

% C:\Users\harelnh\Desktop\winter 2018\project\FastHEVC\matlab\tmp\CTU0
% extract CTU image from srcPath


cu32Struct = struct('cu32',{},'isDiv',{});

jsonCu00Depth = extractfield(jsondecode(fileread(fullfile(srcPath,'CU0_0.json'))),'Depth');

% no division at all
if(jsonCu00Depth == 0)
    return;
end

ctuImg = imread(num2str(cell2mat(fullfile(srcPath,extractfield(dir([srcPath '/CTU-*']),'name')))));

cu32NameMap = {'0_0', '0_32', '32_0', '32_32'};
ctuPosMap = [0 0; 0 32; 32 0; 32 32 ];
for i=1:length(cu32NameMap)
    curJsonName = ['CU' cell2mat(cu32NameMap(i)) '.json'];
    class = 1;
    if ~isempty(dir(fullfile(srcPath,curJsonName)))
        curJsonData = jsondecode(fileread(fullfile(srcPath,curJsonName)));
        curStructIdx = length(cu32Struct)+1;
        
        xPos = ctuPosMap(i,1)+1;
        yPos = ctuPosMap(i,2)+1;
        cu32Struct(curStructIdx).cu32 = ...
            ctuImg(yPos:yPos+31,xPos:xPos+31,:);
        
        if(curJsonData.Depth == 1)
            cu32Struct(curStructIdx).isDiv = 0;
            
        else
            cu32Struct(curStructIdx).isDiv = 1;
        end
    end
    
end

figure;
pos = [1,3,2,4];
for i=[1,2,3,4]
    disp(num2str(pos(i)));
    subplot(2,2,i);imshow(cu32Struct(pos(i)).cu32);title(num2str(i));
end
figure;imshow(ctuImg);
end

