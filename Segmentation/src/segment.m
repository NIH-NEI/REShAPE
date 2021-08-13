function [S] = segment(img,net,xypad,useGpu)
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
    img_size = size(img);
    tile_size = [256 256];
    out_size = 256-2*xypad;
    
    norm_img = (img - mean(img(img>0)))/std(img(img>0));
    %norm_img = (img - imboxfilt(img,255))./stdfilt(img,ones(255));
    pad_img = padarray(norm_img,[xypad xypad],'symmetric');
    pad_size = size(pad_img);
    if ~(mod(pad_size-2*xypad,out_size))==0
        pad_img = pad_img(1:end-xypad,1:end-xypad);
        pad_img = padarray(pad_img,out_size-mod(size(pad_img)-xypad,out_size)+xypad,'symmetric','post');
    end
    pad_size = size(pad_img);
    
    % Create tile indices
    [cindex,rindex] = meshgrid(0:tile_size(2)-1,0:tile_size(1)-1);
    tile_ind = cindex(:)*pad_size(1)+rindex(:);
    
    % Create img indices
    [cindex,rindex] = meshgrid(0:out_size:pad_size(2)-tile_size(2),1:out_size:pad_size(1)-tile_size(1)+1);
    raw_ind = cindex(:)*pad_size(1)+rindex(:);
    
    stack_ind = bsxfun(@plus,tile_ind(:),raw_ind(:)');
    img_pixels = pad_img(stack_ind(:));
    img_pixels = reshape(img_pixels,tile_size(1),tile_size(2),1,[]);
    
    if useGpu
        img_pixels = gpuArray(img_pixels);
    end
    
    disp(['Time to pad and tile the image: ' num2str(toc)]);
    
%% Segment the image
    tic;
    sub_batches = 0:10:size(img_pixels,4);
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
    [seg_size] = (pad_size-2*xypad)/200;
    nRows = seg_size(1);
    nCols = seg_size(2);
    S = single(zeros(seg_size(1)*out_size,seg_size(2)*out_size));
    img_ind = 0;
    ind = 1:out_size;
    for c = 1:nCols
        for r = 1:nRows
            img_ind = img_ind + 1;
            S(ind + out_size*(r-1), ind + out_size*(c-1)) = seg_img(:,:,1,img_ind);
        end
    end
    S = S(1:size(img,1),1:size(img,2));
    disp(['Time to reconstruct image: ' num2str(toc)]);
end