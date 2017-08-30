% player for yuv files.
a = fopen('C:\Users\harelnh\Desktop\spring 2017\project\yuv player\test1rec.yuv','rb');
data = fread(a,'uint8');
height = 480;
width = 832;
y_size = height * width;
u_size = y_size/4;
v_size = y_size/4;

nOFrames = 10;
video = uint8(zeros(width, height, nOFrames));

frame_size = y_size + u_size + v_size;
for i=1:nOFrames
    video(:,:,i) = reshape(data((i - 1)*frame_size + 1:i*frame_size - (u_size + v_size)), width, height); 
end

implay(video);