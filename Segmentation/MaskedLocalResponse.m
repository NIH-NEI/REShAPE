function response = MaskedLocalResponse(I,mask,winSize)
    % Masked Local Response
    % The response is the normalized pixel value = (value-mean)/std
    % The masked local response is the normalized pixel value based on the
    % local distribution of pixels in the mask foreground.
    vals = I.*(mask);
    
    local_count = imboxfilt(double(mask),winSize,'Padding','symmetric','NormalizationFactor',1);
    local_sum = imboxfilt(vals,winSize,'Padding','symmetric','NormalizationFactor',1);
    local_mean = local_sum./local_count;
    local_mean(mask==0) = 0;
    local_sqr_sum = imboxfilt(vals.^2,winSize,'Padding','symmetric','NormalizationFactor',1);
    local_sqr_mean = local_sqr_sum./local_count;
    local_sqr_mean(mask==0) = 0;
    local_std = sqrt(local_sqr_mean-local_mean.^2);
    response = (I - local_mean)./local_std;
    response(mask==0) = 0;
end