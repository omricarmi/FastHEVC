YUV = '/home/omriharel/Desktop/itu601/yuv/mobile.yuv';
FPS = 25;
width = 720;
height = 576;
format = '420';
implay(yuv2mov(YUV,width,height,format),FPS);