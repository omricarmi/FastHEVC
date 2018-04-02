YUV = '/home/omriharel/Desktop/ITUVideos/basketball_big_854x480_30.yuv';

FPS = 50;
width = 854;
height = 480;
format = '420';
implay(yuv2mov(YUV,width,height,format),FPS);