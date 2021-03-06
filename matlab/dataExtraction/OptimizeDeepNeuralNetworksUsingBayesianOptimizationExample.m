%% Deep Learning Using Bayesian Optimization
% This example shows how to apply Bayesian optimization to deep learning
% and find optimal network parameters and training options for
% convolutional neural networks.
%
% To train a deep neural network, you must specify the neural network
% architecture, as well as options of the training algorithm. Selecting and
% tuning these parameters can be difficult and take time. Bayesian
% optimization is an algorithm well suited to optimizing internal
% parameters of classification and regression models. You can use Bayesian
% optimization to optimize functions that are nondifferentiable,
% discontinuous, and time consuming to evaluate. The algorithm internally
% maintains a Gaussian process model of the objective function, and uses
% objective function evaluations to train this model.
%
% This example shows how to:
%
% * Download and prepare the CIFAR-10 data set for network training. This
% data set is one of the most widely used data sets for testing image
% classification models.
% * Specify variables to optimize using Bayesian optimization. These
% variables are options of the training algorithm, as well as parameters of
% the network architecture itself.
% * Define the objective function, which takes the values of the
% optimization variables as inputs, specifies the network architecture and
% training options, trains and validates the network, and saves the trained
% network to disk. The objective function is defined at the end of this
% script.
% * Perform Bayesian optimization by minimizing the classification error on
% the validation set.
% * Load the best network from disk and evaluate it on the test set.

%% Prepare Data
digitDatasetPath = '/home/omriharel/Desktop/divUndiv/data';
testDatasetPath = '/home/omriharel/Desktop/divUndiv/test';
digitData = imageDatastore(digitDatasetPath,...
    'IncludeSubfolders',true,'LabelSource','foldernames');
testDigitData = imageDatastore(testDatasetPath,...
    'IncludeSubfolders',true,'LabelSource','foldernames');
labelCount = countEachLabel(digitData)
trainNumFiles = floor(0.8*min(labelCount.Count));
% valNumFiles = floor(0.2*trainNumFiles);
% testNumFiles = floor(0.2*min(labelCount.Count));
[trainDigitData,valDigitData] = splitEachLabel(digitData,trainNumFiles,'randomize');
% [valDigitData,trainDigitData] = splitEachLabel(trainDigitData,valNumFiles,'randomize');
% [testDigitData,~] = splitEachLabel(testDigitData,testNumFiles,'randomize');


% % Download the CIFAR-10 data set [1]. This data set contains 60,000 images,
% % and each image has the size 32-by-32 and three color channels (RGB). The
% % size of the whole data set is 175 MB. Depending on your internet
% % connection, the download process can take some time.
% cifar10DataDir = pwd;
% url = 'https://www.cs.toronto.edu/~kriz/cifar-10-matlab.tar.gz';
% helperCIFAR10Data.download(url,cifar10DataDir);
% 
% %%
% % Load the CIFAR-10 images as training and test sets. To enable network
% % validation, use 5000 of the training images for validation.
% [trainImages,trainLabels,testImages,testLabels] = helperCIFAR10Data.load(cifar10DataDir);
% 
% idx = randperm(size(trainImages,4),5000);
% valImages = trainImages(:,:,:,idx);
% trainImages(:,:,:,idx) = [];
% valLabels = trainLabels(idx);
% trainLabels(idx) = [];
% 
% %%
% % Display a sample of the training images.
% figure;
% idx = randperm(size(trainImages,4),20);
% for i = 1:numel(idx)
%     subplot(4,5,i);
%     imshow(trainImages(:,:,:,idx(i)));
% end

%% Choose Variables to Optimize
%
% Choose which variables to optimize using Bayesian optimization, and
% specify the ranges to search in. Also, specify whether the variables are
% integers and if the interval should be searched in logarithmic space.
% Optimize the following variables:
%
% * Network depth. This parameter controls the depth of the network. The
% network has three sections, each with |NetworkDepth| identical
% convolutional layers. So the total number of convolutional layers is
% |3*NetworkDepth|. The objective function later in the script takes the
% number of convolutional filters in each layer proportional to
% |1/sqrt(NetworkDepth)|. As a result, the number of parameters and the
% required amount of computation for each iteration are roughly the same
% for different network depths.
% * Initial learning rate. The best learning rate can depend on your data
% as well as the network you are training.
% * Stochastic gradient descent momentum. Momentum adds inertia to the
% parameter updates by having the current update contain a contribution
% proportional to the update in the previous iteration. This results in
% more smooth parameter updates and a reduction of the noise inherent to
% stochastic gradient descent.
% * L2 regularization strength. Use regularization to prevent overfitting.
% Search the space of regularization strength to find a good value. Data
% augmentation and batch normalization also help regularize the network.
optimVars = [
    optimizableVariable('NetworkDepth',[1 3],'Type','integer')
    optimizableVariable('InitialLearnRate',[1e-3 5e-2],'Transform','log')
    optimizableVariable('Momentum',[0.8 0.95])
    optimizableVariable('L2Regularization',[1e-10 1e-2],'Transform','log')];


%% Perform Bayesian Optimization
% Create the objective function for the Bayesian optimizer, using the
% training and validation data as inputs. The objective function trains a
% convolutional neural network and returns the classification error on the
% validation set. This function is defined at the end of this script.
clear functions
ObjFcn = makeObjFcn(trainDigitData,valDigitData);
% ObjFcn = makeObjFcn(trainImages,trainLabels,valImages,valLabels);

%%
% Perform Bayesian optimization by minimizing the classification error on
% the validation set. Specify the maximum number of objective function
% evaluations, and set the maximum total optimization time to two hours. To
% utilize the power of Bayesian optimization better, perform at least 30
% objective function evaluations. After each network finishes training,
% |bayesopt| prints the results to the command window. To train networks in
% parallel on multiple GPUs or CPUs, set the |'UseParallel'| value to
% |true|. The objective function saves the trained networks to disk and
% returns the file names to |bayesopt|. The |bayesopt| function then
% returns the file names in |BayesObject.UserDataTrace|.
BayesObject = bayesopt(ObjFcn,optimVars,...
     'MaxObj',30,... % 30 
    'MaxTime',10*60*60,...
    'IsObjectiveDeterministic',false,...
    'UseParallel',false);

%% Evaluate Final Network
% Load the best network found in the optimization and its validation
% accuracy. Predict the labels of the test set and calculate the test
% error. |bayesopt| determines the best network using the validation set
% without exposing the network to the test set. It is then possible that
% the test error is higher than the validation error.
bestIdx = BayesObject.IndexOfMinimumTrace(end);
fileName = BayesObject.UserDataTrace{bestIdx};
load(fileName);
%%
[predictedLabels,probs] = classify(trainedNet,testDigitData);
testAccuracy = mean(predictedLabels == testDigitData.Labels);
testError = 1 - testAccuracy;

testAccuracy
testError
valError

%%
% Calculate the confusion matrix for the test data and display it as a
% heatmap. The highest confusion is between cats and dogs.
figure
[cmat,classNames] = confusionmat(testDigitData.Labels,predictedLabels);
h = heatmap(classNames,classNames,cmat);
xlabel('Predicted Class');
ylabel('True Class');
title('Confusion Matrix');

%%
% Display some test images together with their predicted classes and the
% probabilities of those classes.
figure
PROB_MAX = 0.6;
PROB_MIN = 0;
idx = randperm(size(testDigitData.Files,1));
for i = 1:25
    while PROB_MIN > max(probs(idx(i),:)) || max(probs(idx(i),:)) > PROB_MAX 
        idx(i) = [];
        if(i > length(idx)) 
            break;
        end
    end
    prob = num2str(100*max(probs(idx(i),:)),3);
    predClass = char(predictedLabels(idx(i)));
    isCorrect = predClass == string(testDigitData.Labels(idx(i)));
    if(i > length(idx)) 
            break;
    end
    subplot(5,5,i)
    imshow(testDigitData.Files{idx(i)});
%     imshow(testImages(:,:,:,idx(i)));
    label = [predClass,', ',prob,'%',', correct:',num2str(isCorrect)];
    title(label)
end

%% Identify Finished Test
isTestExitProperly = true;
save('isTestExitProperly','isTestExitProperly');
%% Objective Function for Optimization
% Define the objective function for optimization. This function performs
% the following steps:
%
% # Takes the values of the optimization variables as inputs. |bayesopt|
% calls the objective function with the current values of the optimization
% variables in a table with each column name equal to the variable name.
% For example, the current value of the network depth is
% |optVars.NetworkDepth|.
% # Defines the network architecture and training options.
% # Trains and validates the network.
% # Saves the trained network, the validation error, and the training
% options to disk.
% # Returns the validation error and the file name of the saved network.
function ObjFcn = makeObjFcn(trainDigitData,valDigitData)
% function ObjFcn = makeObjFcn(trainImages,trainLabels,valImages,valLabels)
ObjFcn = @valErrorFun;
    function [valError,cons,fileName] = valErrorFun(optVars)
        
        %%
        % Define the convolutional neural network architecture.
        %
        % * Add padding to the convolutional layers so that the spatial
        % output size is always the same as the input size.
        % * Each time you down-sample the spatial dimensions by a factor of
        % two using max pooling layers, increase the number of filters by a
        % factor of two. Doing so ensures that the amount of computation
        % required in each convolutional layer is roughly the same.
        % * Choose the number of filters proportional to
        % |1/sqrt(NetworkDepth)|, so that networks of different depths have
        % roughly the same number of parameters and require about the same
        % amount of computation per iteration. To increase the number of
        % network parameters and the overall network flexibility, increase
        % |initialNumFilters|. To train even deeper networks, change the
        % range of the |NetworkDepth| variable.
        % * Use |convBlock(filterSize,numFilters,numConvLayers)| to create
        % a block of |numConvLayers| convolutional layers, each with a
        % specified |filterSize| and |numFilters| filters, and each
        % followed by a batch normalization layer and a ReLU layer. The
        % |convBlock| function is defined at the end of this example.
        imageSize = [32 32 3];
        numClasses = numel(unique(trainDigitData.Labels));
        initialNumFilters = round(32/sqrt(optVars.NetworkDepth));
        layers = [
            imageInputLayer(imageSize)
            
            % The spatial input and output sizes of these convolutional
            % layers are 32-by-32, and the following max pooling layer
            % reduces this to 16-by-16.
            convBlock(3,initialNumFilters,optVars.NetworkDepth)
            maxPooling2dLayer(2,'Stride',2)
            
            % The spatial input and output sizes of these convolutional
            % layers are 16-by-16, and the following max pooling layer
            % reduces this to 8-by-8.
            convBlock(3,2*initialNumFilters,optVars.NetworkDepth)
            maxPooling2dLayer(2,'Stride',2)
            
            % The spatial input and output sizes of these convolutional
            % layers are 8-by-8.
            convBlock(3,4*initialNumFilters,optVars.NetworkDepth)
            
            % Add the fully connected layer and the final softmax and
            % classification layers.
            fullyConnectedLayer(numClasses)
            softmaxLayer
            classificationLayer];
        
        %%
        % Specify options for network training. Optimize the initial
        % learning rate, SGD momentum, and L2 regularization strength. To
        % plot training and validation metrics during training, specify
        % |plotTrainingProgress| as an output function.
        %
        % Specify validation data. Training stops automatically if the
        % validation loss stops improving during training. Choose the
        % |'ValidationFrequency'| value such that |trainNetwork| validates
        % the network three times per epoch. Choose any large value for the
        % maximum number of epochs to train. Training should not reach the
        % final epoch because training stops automatically.
        miniBatchSize = 256;
        numValidationsPerEpoch = 3;
        validationFrequency = floor( size(trainDigitData.Files,1)/miniBatchSize/numValidationsPerEpoch);
        options = trainingOptions('sgdm',...
            'InitialLearnRate',optVars.InitialLearnRate,...
            'Momentum',optVars.Momentum,...
            'MaxEpochs',100, ...
            'L2Regularization',optVars.L2Regularization,...
            'MiniBatchSize',miniBatchSize,...
            'Shuffle','every-epoch',...
            'Verbose',false,...
            'ExecutionEnvironment','gpu',...
            'ValidationData',valDigitData,...
            'ValidationPatience',10,...
            'ValidationFrequency',validationFrequency,...
            'Plots','training-progress');
%             'OutputFcn',@plotTrainingProgressNew,...

%          %%
% %         Use data augmentation to randomly flip the training images along
% %         the vertical axis, and randomly translate them up to four pixels
% %         horizontally and vertically. Data augmentation helps prevent the
% %         network from overfitting and memorizing the exact details of the
% %         training images.
%         pixelRange = [-4 4];
%         imageAugmenter = imageDataAugmenter(...
%             'RandXReflection',true,...
%             'RandXTranslation',pixelRange,...
%             'RandYTranslation',pixelRange);
%         datasource = augmentedImageSource(imageSize,trainImages,trainLabels,...
%             'DataAugmentation',imageAugmenter,...
%             'OutputSizeMode','randcrop');
%         
        datasource = trainDigitData;
        %%
        % Train the network and plot the training progress during training.
        % Close the plot after training finishes.
        trainedNet = trainNetwork(datasource,layers,options);
        close
        
        %%
        %
        % <<../CIFARtrainingplot.png>>
        %
        
        %%
        % After training stops, lower the learning rate by a factor of 10
        % and continue training. This change reduces the noise of the
        % parameter updates and lets the network parameters settle down
        % closer to the minimum of the loss function.
        options = trainingOptions('sgdm',...
            'InitialLearnRate',optVars.InitialLearnRate/10,...
            'Momentum',optVars.Momentum,...
            'MaxEpochs',100, ...
            'MiniBatchSize',miniBatchSize,...
            'L2Regularization',optVars.L2Regularization,...
            'Shuffle','every-epoch',...
            'Verbose',false,...
            'ExecutionEnvironment','gpu',...
            'ValidationData',valDigitData,...
            'ValidationPatience',10,...
            'ValidationFrequency',validationFrequency,...
            'Plots','training-progress');
%                     'OutputFcn',@plotTrainingProgressNew,...
        trainedNet = trainNetwork(datasource,trainedNet.Layers,options);
        close
        
        %%
        % Evaluate the trained network on the validation set, and calculate
        % the validation error.
        predictedLabels = classify(trainedNet,valDigitData);
        valAccuracy = mean(predictedLabels == valDigitData.Labels);
        valError = 1 - valAccuracy;
        
        %%
        % Create a file name containing the validation error, and save the
        % network, validation error, and training options to disk. The
        % objective function returns |fileName| as an output argument, and
        % |bayesopt| returns all the file names in
        % |BayesObject.UserDataTrace|. The additional required output
        % argument |cons| specifies constraints among the variables. There
        % are no variable constraints.
        fileName = num2str(valError,10) + ".mat";
        save(fileName,'trainedNet','valError','options')
        cons = [];
        
        
    end
end

%%
% The |convBlock| function creates a block of |numConvLayers| convolutional
% layers, each with a specified |filterSize| and |numFilters| filters, and
% each followed by a batch normalization layer and a ReLU layer.
function layers = convBlock(filterSize,numFilters,numConvLayers)
layers = [
    convolution2dLayer(filterSize,numFilters,'Padding',(filterSize-1)/2)
    batchNormalizationLayer
    reluLayer];
layers = repmat(layers,numConvLayers,1);
end