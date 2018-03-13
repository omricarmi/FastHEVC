YUV = '/home/omriharel/Desktop/ITUVideos/football_big_854x480_30.yuv';
FPS = 30;
width = 854;
height = 480;
format = 420;
implay(yuv2mov(YUV,width,height,format),FPS);