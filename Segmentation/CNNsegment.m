function [S] = CNNsegment(img,net,opts,batchSize,useGpu)
% segment.m
%
% This function uses a convolutional neural network trained using
% MatConvNet to segment an image. Edges of the image are segmented by
% padding the image using reflection. It is assumed that the size of the
% input image can take on any power of 2. It is also assumed that any
% cropping caused by the networks segmentation is constant regardless of
% image size, and that cropping is uniform in both the horizontal and
% vertical axes.
%
% The third input of the function is how much padding should be applied to
% the edges of the image. If no value is given, this value is estimated by
% segmenting a small portion of the image. The third input is expected to
% be a scalar.
%
% The img input can be a cell array of images to be segmented, provided all
% images are the same size.

%% Initialize the network
    tic;
    img = single(img);
    nn = dagnn.DagNN.loadobj(net);
    nn.mode = 'test';
    pred_ind = nn.getVarIndex('pred');
    if isnan(pred_ind)
        error('Could not find prediction layer.');
    end
    nn.vars(pred_ind).precious = 1;
    if useGpu
        nn.move('gpu');
    else
        nn.move('cpu');
    end
    disp(['Time to load network: ' num2str(toc)])

%% Normalize and Pad the Image
    tic;
    tile_size = opts.prep.imgSize(1);
    out_size = opts.prep.lblSize(1);
    xypad = (tile_size(1)-out_size)/2;
    
    if opts.prep.normImage
        img = MaskedLocalResponse(img,true(size(img)),opts.prep.windowSize);
        img(img>opts.prep.maxNorm) = opts.prep.maxNorm;
        img(img<-opts.prep.maxNorm) = -opts.prep.maxNorm;
    end
    pad_img = padarray(img,[xypad xypad],'symmetric','pre');
    pad_size = size(pad_img);
    pad_size = pad_size(1:2);
    post_pad = ceil((pad_size - xypad)./out_size)*out_size + 2*xypad - pad_size;
    pad_img = padarray(pad_img,post_pad,'symmetric','post');
    pad_size = size(pad_img);
    pad_size = pad_size(1:2);
    
    % Create tile indices
    [cindex,rindex] = meshgrid(0:tile_size-1,0:tile_size-1);
    tile_ind = cindex(:)*pad_size(1)+rindex(:);
    
    % Create img indices
    [cindex,rindex] = meshgrid(0:out_size:pad_size(2)-tile_size,1:out_size:pad_size(1)-tile_size+1);
    raw_ind = cindex(:)*pad_size(1)+rindex(:);
    
    stack_ind = bsxfun(@plus,tile_ind(:),raw_ind(:)');
    img_pixels = single(zeros([tile_size tile_size size(pad_img,3) numel(raw_ind)]));
    for i = 1:size(img_pixels,3)
        I = pad_img(:,:,i);
        I = I(stack_ind(:));
        I = reshape(I,tile_size,tile_size,1,[]);
        img_pixels(:,:,i,:) = I;
    end
    
    if useGpu
        img_pixels = gpuArray(img_pixels);
    end
    
    disp(['Time to pad and tile the image: ' num2str(toc)]);
    
    clear stack_ind raw_ind I pad_img tile_ind
    
%% Segment the image
    tic;
    sub_batches = 0:batchSize:size(img_pixels,4);
    if sub_batches(end) ~= size(img_pixels,4)
        sub_batches(end+1) = size(img_pixels,4);
    end
    
    seg_img = single(zeros(out_size,out_size,1,sub_batches(end)));
    
    for batch = 1:length(sub_batches)-1
        nn.eval({'input',img_pixels(:,:,1,sub_batches(batch)+1:sub_batches(batch+1)),'label',single(zeros(out_size,out_size,1,sub_batches(batch+1)-sub_batches(batch)))});
        if useGpu
            seg_img(:,:,1,sub_batches(batch)+1:sub_batches(batch+1)) = gather(nn.getVar('pred').value)>0;
        else
            seg_img(:,:,1,sub_batches(batch)+1:sub_batches(batch+1)) = (nn.getVar('pred').value)>0;
        end
        disp(['Finished batch ' num2str(batch) ' of ' num2str(length(sub_batches)-1) '. Total duration: ' num2str(toc)]);
    end
    disp(['Time to segment the image: ' num2str(toc)])
    
%% Construct segmented image
    tic;
    [seg_size] = (pad_size-2*xypad)./out_size;
    nRows = seg_size(1);
    nCols = seg_size(2);
    S = single(zeros(seg_size(1)*out_size,seg_size(2)*out_size));
    img_ind = 0;
    ind = 1:out_size;
    for c = 1:nCols
        for r = 1:nRows
            img_ind = img_ind + 1;
            try
                S(ind + out_size*(r-1), ind + out_size*(c-1)) = seg_img(:,:,1,img_ind);
            catch err
                rethrow(err)
            end
        end
    end
    S = S(1:size(img,1),1:size(img,2));
    disp(['Time to reconstruct image: ' num2str(toc)]);
end