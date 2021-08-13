function [ net, outVar, outChan ] = createInception(net,inVar,outVar,inChan,sizes,redDepth,depth,varargin)

if isempty(varargin)
    useBatchNorm = false;
elseif nargin>8
    error('Too many inputs');
else
    useBatchNorm = varargin{1};
end

% Check to see if sizes are valid. Sizes must be odd.
if sum(mod(sizes,2))~=length(sizes)
    error('Invalid size.')
end

if length(redDepth)>length(depth)+1 || length(redDepth)<length(depth)
    error('Number of elements in [redDepth] and [depth] do not match.')
end

if length(depth) == 1
    depth = ones(length(sizes),1)*depth;
    redDepth = ones(length(sizes),1)*redDepth;
end

if length(depth)>length(sizes)+1 || length(depth)<length(sizes)
    error('Depth does not match sizes.');
end

ind = find(sizes==1);
if ~isempty(ind) && depth(ind)~=redDepth(ind)
    error('For 1x1 convolution, redDepth~=depth.')
end

% Initialize outputs
outputs = {};
outChan = sum(depth);
if length(redDepth)>length(depth)
    outChan = outChan + redDepth(end);
end

% Create Convolutional Layers
for i = 1:length(sizes)
    nb = [outVar '_' num2str(i) '_'];
    
    if inChan==1
        d = 1;
        dnbr = inVar;
        if sizes(i) == 1
            continue;
        end
    else
        d = redDepth(i);
        % 1x1 depth scaling
        dnb = [nb 'd'];
        % Convolution
        depthscale = dagnn.Conv('size',[1 1 inChan redDepth(i)],...
                                'pad',0,'stride',1,'hasBias',true);
        net.addLayer(dnb,depthscale,...
                     {inVar},...             % Input variable
                     {dnb},...               % Output variable
                     {[dnb 'f'] [dnb 'b']}); % Filters and biases
        net = initParams(net);
        
            
        if useBatchNorm
            dnb = [nb 'bn'];
            net.addLayer(dnb,dagnn.BatchNorm('numChannels',redDepth(i)),{[dnb(1:end-2) 'd']},{dnb},{[dnb 'f'] [dnb 'b'] [dnb 'm']});
            net = initParams(net);
        end
        
        % ReLU
        dnbr = [dnb 'r'];
        net.addLayer(dnbr,dagnn.ReLU(),...
                     {dnb},...               % Input variable
                     {dnbr});           % Output variable

        if sizes(i) == 1
            outputs(end+1) = {[dnb 'r']};
            continue;
        end
    end
             
    % Spatial layer
    snb = [nb 's'];
    % Convolution
    spatial = dagnn.Conv('size',[sizes(i) sizes(i) d depth(i)],...
                         'pad',floor(sizes(i)/2),...
                         'stride',1,...
                         'hasBias',true);
    net.addLayer(snb,spatial,...
                 {dnbr},...              % Input layer
                 {snb},...               % Output layer
                 {[snb 'f'] [snb 'b']}); % Filters and biases
    net = initParams(net);
    
    if useBatchNorm && d==1
        snb = [nb 'bn'];
        net.addLayer(snb,dagnn.BatchNorm('numChannels',depth(i)),{[snb(1:end-2) 's']},{snb},{[snb 'f'] [snb 'b'] [snb 'm']});
        net = initParams(net);
    end
    
    % ReLU
    net.addLayer([snb 'r'],dagnn.ReLU(),...
                 {snb},...         % Input layer
                 {[snb 'r']});           % Output layer
             
    outputs(end+1) = {[snb 'r']};
end

if length(redDepth) > length(sizes) && inChan > 1
    nb = [outVar '_' num2str(length(sizes)+1) '_'];
    
    % 1x1 depth scaling
    dnb = [nb 'd'];
    % Convolution
    depthscale = dagnn.Conv('size',[1 1 inChan redDepth(end)],...
                            'pad',0,'stride',1,'hasBias',true);
    net.addLayer(dnb,depthscale,...
                 {inVar},...             % Input variable
                 {dnb},...               % Output variable
                 {[dnb 'f'] [dnb 'b']}); % Filters and biases
    net = initParams(net);
    % ReLU
    net.addLayer([dnb 'r'],dagnn.ReLU(),...
                 {dnb},...               % Input variable
                 {[dnb 'r']});           % Output variable
             
    % Spatial layer
    snb = [nb 'mp'];
    % Pooling
    spatial = dagnn.Pooling('method', 'max', 'poolSize', [3 3],'pad', 1, 'stride', 1);
    net.addLayer(snb,spatial,...
                 {[dnb 'r']},...         % Input layer
                 {snb});                 % Output layer
             
    outputs(end+1) = {snb};
end
    
% Concatenate layers
outVar = [outVar 'cat'];
net.addLayer(outVar,...            % Name the layer the same as variable
             dagnn.Concat(),...    % Concatenation layer
             outputs,...           % Outputs of parallel layers
             {outVar});              % Name of output variable
end

function net = initParams(net)
p = net.getParamIndex(net.layers(end).params) ;
params = net.layers(end).block.initParams() ;
switch net.device
    case 'cpu'
        params = cellfun(@gather, params, 'UniformOutput', false) ;
    case 'gpu'
        params = cellfun(@gpuArray, params, 'UniformOutput', false) ;
end
disp(['Initializing: ' net.layers(end).name]);
[net.params(p).value] = deal(params{:}) ;
end