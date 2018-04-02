%% Visualize Activations of a Convolutional Neural Network
% This example shows how to feed an image to a convolutional neural network
% and display the activations of different layers of the network. Examine
% the activations and discover which features the network learns by
% comparing areas of activation with the original image. Find out that
% channels in earlier layers learn simple features like color and edges,
% while channels in the deeper layers learn complex features like eyes.
% Identifying features in this way can help you understand what the network
% has learned.
%
% The example requires Neural Network Toolbox(TM), Image Processing
% Toolbox(TM) and the Neural Network Toolbox(TM) Model _for AlexNet
% Network_.

%% Load Pretrained Network and Data
% Load the popular pretrained network _AlexNet_. If Neural Network
% Toolbox(TM) Model _for AlexNet Network_ is not installed, then the
% software provides a download link.
% net = alexnet;
close all;clear all;clc
net = load('0.100794934.mat');
net = net.trainedNet;


%%
% Read and show an image. Save its size for future use.
% im = imread(fullfile(matlabroot,'examples','nnet','face.jpg'));
im = imread('buildings1_F0_CTU-76-X-256-Y-384_CU_0_32.png');
imshow(im)
imgSize = size(im);
imgSize = imgSize(1:2);

%% View Network Architecture
% Display the layers of the network to see which layers you can look at.
% The convolutional layers perform convolutions with learnable parameters.
% The network learns to identify useful features, often with one feature
% per channel. Observe that the first convolutional layer has 96 channels.
net.Layers

%%
% The Image Input layer specifies the input size. You can resize the image
% before passing it through the network, but the network also can process
% larger images. If you feed the network larger images, the activations
% also become larger. However, since the network is trained on images of
% size 227-by-227, it is not trained to recognize objects or features
% larger than that size.

%% Show Activations of First Convolutional Layer
% Investigate features by observing which areas in the convolutional layers
% activate on an image and comparing with the corresponding areas in the
% original images. Each layer of a convolutional neural network consists of
% many 2-D arrays called _channels_. Pass the image through the network and
% examine the output activations of the |conv1| layer.
act1 = activations(net,im,'conv_4','OutputAs','channels');

%%
% The activations are returned as a 3-D array, with the third dimension
% indexing the channel on the |conv1| layer. To show these activations
% using the |montage| function, reshape the array to 4-D. The third
% dimension in the input to |montage| represents the image color. Set the
% third dimension to have size 1 because the activations do not have color.
% The fourth dimension indexes the channel.
sz = size(act1);
act1 = reshape(act1,[sz(1) sz(2) 1 sz(3)]);

%%
% Now you can show the activations. Each activation can take any value, so
% normalize the output using |mat2gray|. All activations are scaled so that
% the minimum activation is 0 and the maximum is 1. Display a montage of
% the 96 images on an 8-by-12 grid, one for each channel in the layer.
montage(mat2gray(act1),'Size',[10 10])

%% Investigate the Activations in Specific Channels
% Each square in the montage of activations is the output of a channel in
% the |conv1| layer. White pixels represent strong positive activations and
% black pixels represent strong negative activations. A channel that is
% mostly gray does not activate as strongly on the input image. The
% position of a pixel in the activation of a channel corresponds to the
% same position in the original image. A white pixel at some location in a
% channel indicates that the channel is strongly activated at that
% position.
%
% Resize the activations in channel 32 to have the same size as the
% original image and display the activations.
act1ch32 = act1(:,:,:,46);
act1ch32 = mat2gray(act1ch32);
act1ch32 = imresize(act1ch32,imgSize);
imshowpair(im,act1ch32,'montage')

%%
% You can see that this channel activates on red pixels, because the whiter
% pixels in the channel correspond to red areas in the original image.

%% Find the Strongest Activation Channel
% You also can try to find interesting channels by programmatically
% investigating channels with large activations. Find the channel with the
% largest activation using the |max| function, resize, and show the
% activations.
[maxValue,maxValueIndex] = max(max(max(act1)));
act1chMax = act1(:,:,:,maxValueIndex);
act1chMax = mat2gray(act1chMax);
act1chMax = imresize(act1chMax,imgSize);
imshowpair(im,act1chMax,'montage')

%%
% Compare to the original image and notice that this channel activates on
% edges. It activates positively on light left/dark right edges, and
% negatively on dark left/light right edges.

%% Investigate a Deeper Layer
% Most convolutional neural networks learn to detect features like color
% and edges in their first convolutional layer. In deeper convolutional
% layers, the network learns to detect more complicated features. Later
% layers build up their features by combining features of earlier layers.
% Investigate the |conv5| layer in the same way as the |conv1| layer.
% Calculate, reshape, and show the activations in a montage.
act5 = activations(net,im,'conv5','OutputAs','channels');
sz = size(act5);
act5 = reshape(act5,[sz(1) sz(2) 1 sz(3)]);
montage(imresize(mat2gray(act5),[48 48]))

%%
% There are too many images to investigate in detail, so focus on some of
% the more interesting ones. Display the strongest activation in the
% |conv5| layer.
[maxValue5,maxValueIndex5] = max(max(max(act5)));
act5chMax = act5(:,:,:,maxValueIndex5);
imshow(imresize(mat2gray(act5chMax),imgSize))

%%
% In this case, the maximum activation channel is not as interesting for
% detailed features as some others, and shows strong negative (dark) as
% well as positive (light) activation. This channel is possibly focusing on
% faces.
%
% In the montage of all channels, there are channels that might be
% activating on eyes. Investigate channels 3 and 5 further.
montage(imresize(mat2gray(act5(:,:,:,[3 5])),imgSize))

%%
% Many of the channels contain areas of activation that are both light and
% dark. These are positive and negative activations, respectively. However,
% only the positive activations are used because of the rectified linear
% unit (ReLU) that follows the |conv5| layer. To investigate only positive
% activations, repeat the analysis to visualize the activations of the
% |relu5| layer.
act5relu = activations(net,im,'relu5','OutputAs','channels');
sz = size(act5relu);
act5relu = reshape(act5relu,[sz(1) sz(2) 1 sz(3)]);
montage(imresize(mat2gray(act5relu(:,:,:,[3 5])),imgSize))

%%
% Compared to the activations of the |conv5| layer, the activations of the
% |relu5| layer clearly pinpoint areas of the image that have strong facial
% features.

%% Test Whether a Channel Recognizes Eyes
% Check whether channels 3 and 5 of the |relu5| layer activate on eyes.
% Input a new image with one closed eye to the network and compare the
% resulting activations with the activations of the original image.

%%
% Read and show the image with one closed eye and compute the activations
% of the |relu5| layer.
imClosed = imread(fullfile(matlabroot,'examples','nnet','face-eye-closed.jpg'));
imshow(imClosed)

act5Closed = activations(net,imClosed,'relu5','OutputAs','channels');
sz = size(act5Closed);
act5Closed = reshape(act5Closed,[sz(1),sz(2),1,sz(3)]);

%%
% Plot the images and activations in one figure.
channelsClosed = repmat(imresize(mat2gray(act5Closed(:,:,:,[3 5])),imgSize),[1 1 3]);
channelsOpen = repmat(imresize(mat2gray(act5relu(:,:,:,[3 5])),imgSize),[1 1 3]);
montage(cat(4,im,channelsOpen*255,imClosed,channelsClosed*255));
title('Input Image, Channel 3, Channel 5');

%%
% You can see from the activations that both channels 3 and 5 activate on
% individual eyes, and to some degree also on the area around the mouth.
%
% The network has never been told to learn about eyes, but it has learned
% that eyes are a useful feature to distinguish between classes of images.
% Previous machine learning approaches often manually designed features
% specific to the problem, but these deep convolutional networks can learn
% useful features for themselves. For example, learning to identify eyes
% could help the network distinguish between a leopard and a leopard print
% rug.