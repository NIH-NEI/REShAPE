classdef L2Loss < dagnn.Loss
%LOSSREGUL Loss function for L2 loss between targets and activations
%   Detailed explanation goes here

properties
    type = 'L2'
end

methods
    function outputs = forward(obj, inputs, params)
      outputs{1} = vl_nnL2(inputs{1}, inputs{2}, [],'loss',obj.type) ;
      n = obj.numAveraged ;
      m = n + size(inputs{1},4) ;
      obj.average = (n * obj.average + gather(outputs{1})) / m ;
      obj.numAveraged = m ;
    end

    function [derInputs, derParams] = backward(obj, inputs, params, derOutputs)

      derInputs{1} = vl_nnL2(inputs{1}, inputs{2}, derOutputs{1},'loss',obj.type,'instanceWeights',inputs{3}) ;
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