classdef Fscore < dagnn.Loss
% Fscore error
%
% The derivative of this function is flat. This should be used for
% estimating accuracy using the formula for f-score. Do not use this for
% training a network.
%
% The formula for the f-score is:
%   f = (1+beta)^2.*(p.*r)./((beta^2).*p + r)
% where p is precision and r is recall. You can specify the beta property
% of this class. The default is the f1 score (beta = 1), aka dice index.
%
% Only one value per training image is given. The values are then averaged
% to give an average fscore.

properties
    beta = 1
end

methods
    function outputs = forward(obj, inputs, params)
      outputs{1} = vl_nnfscore(inputs{1}, inputs{2}, [],'beta',obj.beta) ;
      n = obj.numAveraged ;
      m = n + size(inputs{1},4) ;
      obj.average = (n * obj.average + gather(outputs{1})) / m ;
      obj.numAveraged = m ;
    end

    function [derInputs, derParams] = backward(obj, inputs, params, derOutputs)

      if length(inputs)>2
        derInputs{1} = obj.vl_nnfscore(inputs{1}, inputs{2}, derOutputs{1},'beta',obj.beta,'instanceWeights',inputs{3}) ;
      else
        derInputs{1} = obj.vl_nnfscore(inputs{1}, inputs{2}, derOutputs{1},'beta',obj.beta) ;
      end
      derInputs{2} = [] ;
      derInputs{3} = [];
      derParams = {} ;
    end

    function outputSizes = getOutputSizes(obj, inputSizes, paramSizes)
      outputSizes{1} = [1 1 1 inputSizes{1}(4)] ;
    end

    function rfs = getReceptiveFields(obj)
      % the receptive field depends on the dimension of the variables
      % which is not known until the network is run
      rfs(1,1).size = [NaN NaN] ;
      rfs(1,1).stride = [NaN NaN] ;
      rfs(1,1).offset = [NaN NaN] ;
      rfs(2,1) = rfs(1,1) ;
    end
    
end

end