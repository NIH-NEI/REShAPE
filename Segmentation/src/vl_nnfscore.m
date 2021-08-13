function Y = vl_nnfscore(X,c,dzdy,varargin)
    opts = struct;
    opts.beta = 1;
    opts.instanceWeights = ones(size(c));
    opts = vl_argparse(opts,varargin);

    if nargin <= 2 || isempty(dzdy)
        Xr = reshape(X,[],1,1,size(X,4));
        cr = reshape(c,[],1,1,size(c,4));
        TP = sum((Xr>0) & (cr>0)); % true positives
        FP = sum((Xr>0) & (cr<0)); % false positives
        FN = sum((Xr<0) & (cr>0)); % false negatives
        p = TP./(TP + FP);         % precision
        r = TP./(TP + FN);         % recall
        Y = (1+opts.beta^2).*(p.*r)./((opts.beta^2).*p + r);
        Y(isnan(Y)) = 0;
        Y = sum(Y,4);
    else
        Y = obj.zerosLike(X);
    end
end

function y = zerosLike(x)
    if isa(x,'gpuArray')
        y = gpuArray.zeros(size(x),classUnderlying(x)) ;
    else
        y = zeros(size(x),'like',x) ;
    end
end

