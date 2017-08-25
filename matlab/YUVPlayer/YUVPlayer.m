YUV = '/Users/omricarmi/Technion/ProjectA/ITUVideos/BasketballDrill_832x480_50.yuv';
FPS = 50;
width = 832;
height = 480;
format = 420;
implay(yuv2mov(YUV,width,height,format),FPS);