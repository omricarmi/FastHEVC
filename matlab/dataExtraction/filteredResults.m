%% use this part before first time on next section
% NN_FILE = '0.09897504456.mat';
% load(NN_FILE);
% [predictedLabels,probs] = classify(trainedNet,testDigitData);
%%
% Display some test images together with their predicted classes and the
% probabilities of those classes.
PROB_MAX = 1;
PROB_MIN = 0;
idx = randperm(size(testDigitData.Files,1));
figure
for i = 1:25
    while PROB_MIN > probs(idx(i)) || probs(idx(i)) > PROB_MAX 
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