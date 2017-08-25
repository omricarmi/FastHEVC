%yuv2mov creates a Matlab-Movie from a YUV-File.
%	yuv2mov('Filename', width, height, format) reads the specified file
%	using width and height for resolution and format for YUV-subsampling.
%	
%	Filename --> Name of File (e.g. 'Test.yuv')
%   width    --> width of a frame  (e.g. 352)
%   height   --> height of a frame (e.g. 280)
%   format   --> subsampling rate ('400','411','420','422' or '444')
%
%example: mov = yuv2mov('Test.yuv',352,288,'420');

function mov = yuv2mov(File,width,height,format)

    mov = [];
    %set factor for UV-sampling
    fwidth = 0.5;
    fheight= 0.5;
    if strcmp(format,'400')
        fwidth = 0;
        fheight= 0;
    elseif strcmp(format,'411')
        fwidth = 0.25;
        fheight= 1;
    elseif strcmp(format,'420')
        fwidth = 0.5;
        fheight= 0.5;
    elseif strcmp(format,'422')
        fwidth = 0.5;
        fheight= 1;
    elseif strcmp(format,'444')
        fwidth = 1;
        fheight= 1;
    else
        display('Error: wrong format');
    end
    %get Filesize and Framenumber
    filep = dir(File); 
    fileBytes = filep.bytes; %Filesize
    clear filep
    framenumber = fileBytes/(width*height*(1+2*fheight*fwidth)); %Framenumber
    if mod(framenumber,1) ~= 0
        display('Error: wrong resolution, format or filesize');
    else
        h = waitbar(0,'Please wait ... ');
        %read YUV-Frames
        for cntf = 1:1:framenumber
            waitbar(cntf/framenumber,h);
            YUV = loadFileYUV(width,height,cntf,File,fheight,fwidth);
            RGB = ycbcr2rgb(YUV); %Convert YUV to RGB
            mov(cntf).cdata = RGB;
            mov(cntf).colormap = [];
        end
        close(h);
    end
end
