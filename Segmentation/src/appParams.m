function [opts] = appParams()

opts = struct;

% GPU Usage
opts.useGpu = false;
opts.verbose = false;

% Set to empty matrix to create a new folder for every training run
opts.expDir = 'New Data'; 

opts.holdout = 0.2; % Proportion of images used for validation during training
opts.augment = 1; % Number of replicates of jittered data there should be

% Jitter options
opts.flip_horizontal = true;
opts.flip_vertical = true;
opts.poisson_noise = true;

% Training options
opts.train.batchSize = 10;
opts.train.numSubBatches = 1;
opts.train.numEpochs = 500 ;
opts.train.continue = true ;
opts.train.gpus = [] ;
opts.train.learningRate = 0.001;
opts.train.expDir = opts.expDir;

end