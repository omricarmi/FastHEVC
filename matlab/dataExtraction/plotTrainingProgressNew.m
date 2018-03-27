function plotTrainingProgressNew(info)

persistent fhdl plotObjAcc plotObjLoss plotObjLearnRate

if info.State == "start"
        fhdl = figure;
        pos = fhdl.Position;
        pos(3) = pos(3)*1.8;
        pos(2) = pos(2)/5; 
        pos(4) = pos(4)*1.8;
        fhdl.Position = pos;
        
        % Create subplots for each of the animated lines. 
        ax = subplot(3,1,1);
        ylim([0,100])
        ax.YMinorGrid = 'on';
        ax.YAxisLocation = 'right';
        ax.XTickLabel = {};        
        ylabel("Accuracy (%)")
        plotObjAcc = animatedline;
          
        ax = subplot(3,1,2);
        ylim([0,Inf])
        ax.YMinorGrid = 'on'; 
        ax.YAxisLocation = 'right';  
        ax.XTickLabel = {};        
        plotObjLoss = animatedline('Color','r');
        ylabel("Loss")
        
        ax = subplot(3,1,3);
        ax.YMinorGrid = 'on'; 
        ax.YAxisLocation = 'right';
        plotObjLearnRate = animatedline('Color','b');
        xlabel("Iteration")
        ylabel("Learning rate")
        %ax.YScale = 'log'
        %axis tight
        ylim([0,Inf])
        subplot(3,1,1) 
        
elseif info.State == "iteration"
    % Add the current information to the plotted lines at every iteration.
    title(['Network training progress: Epoch ',num2str(info.Epoch)])
    addpoints(plotObjAcc,info.Iteration,info.TrainingAccuracy)
    addpoints(plotObjLoss,info.Iteration,double(gather((info.TrainingLoss))))
    addpoints(plotObjLearnRate,info.Iteration,double(info.BaseLearnRate))
    drawnow limitrate nocallbacks
    
elseif info.State == "done" 
end

end










