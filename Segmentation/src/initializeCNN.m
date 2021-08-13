function nn = initializeCNN()

nn = dagnn.DagNN();

%% Unit 1 (Downsample)
% Inception Layer
% Input:  256x256x1
% Output: 256x256x64
[nn, u0i, u0c] = createInception(nn,'input','u0',1,[3 5],[1 1],[32 32],true);

% Inception Layer
% Input:  256x256x64
% Output: 256x256x64
[nn, u1i, u1c] = createInception(nn,u0i,'u1',u0c,[1 3 5],[32 32 32],[32 16 16]);

% Max Pooling
% Stride: 2
% Input:  128x128x64
u1_m = dagnn.Pooling('method', 'max', 'poolSize', [2 2],'pad', 0, 'stride', 2);
nn.addLayer('u1_m', u1_m, {u1i}, {'u1_m'});
                       
%% Unit 2 (Downsample)
% Inception Layer
% Input:  128x128x64
% Output: 128x128x128
[nn, u2i, u2c] = createInception(nn,'u1_m','u2',u1c,[1 3 5],[64 32 32],[64 32 32]);

% Max Pooling
% Stride: 2
% Input:  64x64x128
u2_m = dagnn.Pooling('method', 'max', 'poolSize', [2 2],'pad', 0, 'stride', 2);
nn.addLayer('u2_m', u2_m, {u2i}, {'u2_m'});

%% Unit 3 (Downsample)
% Inception Layer
% Input:  64x64x128
% Output: 64x64x256
[nn, u3i, u3c] = createInception(nn,'u2_m','u3',u2c,[1 3 5],[128 64 32],[128 64 64]);

% Max Pooling
% Stride: 2
% Input:  32x32x256
u3_m = dagnn.Pooling('method', 'max', 'poolSize', [2 2],'pad', 0, 'stride', 2);
nn.addLayer('u3_m', u3_m, {u3i}, {'u3_m'});

%% Unit 3a (Downsample)
% Inception Layer
% Input:  32x32x256
% Output: 32x32x512
[nn, u3ai, u3ac] = createInception(nn,'u3_m','u3a',u3c,[1 3 5],[192 128 64],[192 192 128]);

% Max Pooling
% Stride: 2
% Input: 16x16x512
u3a_m = dagnn.Pooling('method', 'max', 'poolSize', [2 2],'pad', 0, 'stride', 2);
nn.addLayer('u3a_m', u3a_m, {u3ai}, {'u3a_m'});

%% Unit 4 (Deep Inception)
% Inception Layer 1
% Input:  16x16x512
% Output: 16x16x512
[nn, u41i, u41c] = createInception(nn,'u3a_m','u41',u3ac,[1 3 5],[128 64 64 128],[128 192 64]);

% Inception Layer 2
% Input:  16x16x512
% Output: 16x16x512
[nn, u42i, u42c] = createInception(nn,u41i,'u42',u41c,[1 3 5],[128 64 64 128],[128 192 64]);

% Inception Layer 3
% Input:  16x16x512
% Output: 16x16x512
[nn, u43i, u43c] = createInception(nn,u42i,'u43',u42c,[1 3 5],[128 64 64 128],[128 192 64],true);

%% Unit 5a (Upsample and Concatenate)
% Convolutional Transpose
% Input:  16x16x512
% Output: 32x32x512
filters = single(bilinear_u(6, u43c, u43c)) ;
u5a_ct = dagnn.ConvTranspose('size', size(filters),'upsample', 2, 'crop', 2, ...
                            'numGroups', u43c, 'hasBias', false);
nn.addLayer('u5a_ct',u5a_ct,{u43i},{'u5a_ct'},{'u5a_ctf'});
nn = initParams(nn);

f = nn.getParamIndex('u5a_ctf') ;
nn.params(f).value = filters ;
nn.params(f).learningRate = 0 ;
nn.params(f).weightDecay = 1 ;

% Concatenate Layer
% Output: 32x32x1024
nn.addLayer('u5a_cc',dagnn.Concat(),{'u5a_ct' u3ai},{'u5a_cc'});

% Inception Layer
% Input:  32x32x1024
% Output: 32x32x512
[nn, u5ai, u5ac] = createInception(nn,'u5a_cc','u5a',1024,[1 3 5],[256 256 128 64],[256 128 64]);

%% Unit 5 (Upsample and Concatenate)
% Convolutional Transpose
% Input:  32x32x512
% Output: 64x64x256
filters = single(bilinear_u(6, u5ac, u5ac)) ;
u5_ct = dagnn.ConvTranspose('size', size(filters),'upsample', 2, 'crop', 2, ...
                            'numGroups', u5ac/2, 'hasBias', false);
nn.addLayer('u5_ct',u5_ct,{u5ai},{'u5_ct'},{'u5_ctf'});
nn = initParams(nn);

f = nn.getParamIndex('u5_ctf') ;
nn.params(f).value = filters ;
nn.params(f).learningRate = 0 ;
nn.params(f).weightDecay = 1 ;

% Concatenate Layer
% Output: 64x64x512
nn.addLayer('u5_cc',dagnn.Concat(),{'u5_ct' u3i},{'u5_cc'});

% Inception Layer
% Input:  64x64x512
% Output: 64x64x512
[nn, u5i, u5c] = createInception(nn,'u5_cc','u5',512,[1 3 5],[128 128 64 32],[128 64 32]);

%% Unit 6 (Upsample and Concatenate)
% Convolutional Transpose
% Input:  32x32x256
% Output: 64x64x128
filters = single(bilinear_u(6, u5c, u5c)) ;
u6_ct = dagnn.ConvTranspose('size', size(filters),'upsample', 2, 'crop', 2, ...
                            'numGroups', u5c/2, 'hasBias', false);
nn.addLayer('u6_ct',u6_ct,{u5i},{'u6_ct'},{'u6_ctf'});

f = nn.getParamIndex('u6_ctf') ;
nn.params(f).value = filters ;
nn.params(f).learningRate = 0 ;
nn.params(f).weightDecay = 1 ;

% Concatenate Layer
% Output: 64x64x256
nn.addLayer('u6_cc',dagnn.Concat(),{'u6_ct' u2i},{'u6_cc'});

% Inception Layer
% Input:  64x64x256
% Output: 64x64x128
[nn, u6i, u6c] = createInception(nn,'u6_cc','u6',256,[1 3 5],[64 64 32 16],[64 32 16]);

%% Unit 7 (Upsample and Concatenate)
% Convolutional Transpose
% Input:  64x64x128
% Output: 128x128x64
filters = single(bilinear_u(6, u6c, u6c)) ;
u7_ct = dagnn.ConvTranspose('size', size(filters),'upsample', 2, 'crop', 2, ...
                            'numGroups', u6c/2, 'hasBias', false);
nn.addLayer('u7_ct',u7_ct,{u6i},{'u7_ct'},{'u7_ctf'});

f = nn.getParamIndex('u7_ctf') ;
nn.params(f).value = filters ;
nn.params(f).learningRate = 0 ;
nn.params(f).weightDecay = 1 ;

% Concatenate Layer
% Output: 128x128x128
nn.addLayer('u7_cc',dagnn.Concat(),{'u7_ct' u1i},{'u7_cc'});

% Inception Layer
% Input:  128x128x128
% Output: 128x128x128
[nn, u7i, u7c] = createInception(nn,'u7_cc','u7',128,[1 3 5],[64 32 16 16],[64 32 16],true);
                       
%% Unit 8 (Classification and Loss)
% Final Convolution
% Input:  256x256x128
% Output: 254x254x256
u8_c1 = dagnn.Conv('size',[3 3 128 256],'pad',0,'stride',1,'hasBias',true);
nn.addLayer('u8_c1',u8_c1,{u7i},{'u8_c1'},{'u8_c1f' 'u8_c1b'});
nn = initParams(nn);

% Batch Normalization
u8_bn = dagnn.BatchNorm('numChannels',256);
nn.addLayer('u8_bn',u8_bn,{'u8_c1'},{'u8_bn'},{'u8_bnf' 'u8_bnb' 'u8_bnm'});
nn = initParams(nn);

% ReLU
nn.addLayer('u8_r1',dagnn.ReLU(),{'u8_bn'},{'u8_r1'});

% Dropout
nn.addLayer('u8_do',dagnn.DropOut('rate',0.5),{'u8_r1'},{'u8_do'});

% Prediction
% Input:  254x254x256
% Output: 250x250x1
u8_cls = dagnn.Conv('size',[5 5 256 1],'pad',0,'stride',1,'hasBias',true);
nn.addLayer('u8_cls',u8_cls,{'u8_do'},{'u8_cls'},{'u8_clsf' 'u8_clsb'});
nn = initParams(nn);

u8_crop = dagnn.Crop('crop',[26 225 26 225]);
nn.addLayer('u8_crop',u8_crop,{'u8_cls' 'label'},{'pred'});
nn = initParams(nn);

e = dagnn.Loss('loss', 'binaryerror');
e.opts = {'instanceWeights',ones(200)/(200^2)};
nn.addLayer('error', e, {'pred','label'}, 'error');
sm = dagnn.Loss('loss', 'logistic');
sm.opts = {'instanceWeights',ones(200)/(200^2)};
nn.addLayer('loss', sm, {'pred','label'}, 'objective');

%% Initialize Parameters         
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
