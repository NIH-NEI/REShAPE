function y = get_batch(imdb,batch)
%GET_BATCH Summary of this function goes here
%   Detailed explanation goes here
    if imdb.meta.useGpu
        im = gpuArray(imdb.images.data(:,:,1,batch));
    else
        im = imdb.images.data(:,:,1,batch);
    end
    labels = imdb.images.label(:,:,1,batch);
    
    if imdb.images.label(batch(1))==1
        im = im/2 + poissrnd(im)/2;
        
        if rand > 0.5
            im = fliplr(im);
            labels = fliplr(labels);
        end
        if rand > 0.5
            im = flipud(im);
            labels = flipud(labels);
        end
    end
    
    for i = 1:size(im,4)
        im(:,:,1,i) = im(:,:,1,i) - imdb.meta.mean(imdb.images.imgId(batch(i)));
        im(:,:,1,i) = im(:,:,1,i)/imdb.meta.std(imdb.images.imgId(batch(i)));
    end
    
    y = {'input', im, 'label', labels} ;
end

