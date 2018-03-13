YUV = '/Users/omricarmi/Technion/projectA/ITUVideos/stefan_cif_352x288_30.yuv';
FPS = 30;
width = 352;
height = 288;
format = 420;
implay(yuv2mov(YUV,width,height,format),FPS);