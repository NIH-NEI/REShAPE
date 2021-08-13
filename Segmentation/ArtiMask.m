function [bitmask] = ArtiMask(I,cutoff)

    % This function returns coordinates of all 255-pixels connected to the
    % input pixel at (xc,yc) vertically, hoprizontally or diagonally.
    function [yarr, xarr, sz] = FindArea(S, yc, xc, bkg, fgd)
        inext = 2;
        icurr = 1;
        yarr(icurr) = yc;
        xarr(icurr) = xc;
        S(yc, xc) = fgd;
        while icurr < inext
            y0 = yarr(icurr);
            x0 = xarr(icurr);
            icurr = icurr + 1;
            for yy = y0-1:y0+1
                for xx = x0-1:x0+1
                    if S(yy,xx) == bkg
                        yarr(inext) = yy;
                        xarr(inext) = xx;
                        S(yy, xx) = fgd;
                        inext = inext + 1;
                    end
                end
            end
        end
        sz = inext-1;
    end

    scaledown = 16;
    minarea = 200;

    imgH = size(I,1);
    imgW = size(I,2);
    
    % The standard way of computing means and SDs does not work
    % for images with large empty (0-pixel) areas.
    % xmean = mean(I(:));
    % stdev = std(single(I(:)));

    % Exclude background (0-pixels) from mean/SD calculation
    maxbins = 256;
    if isa(I, 'uint16')
        maxbins = 65536;
    end
    [counts, locs] = imhist(I, maxbins);
    pixcount = imgH*imgW;
    % Zreo first 5 bins in the histogram,
    % subtract their counts from pixcount.
    for i = 1:5
        pixcount = pixcount - counts(i);
        counts(i) = 0;
    end
    if pixcount < minarea
        % Empty image, don't mask out anything
        bitmask = zeros(size(I,1), size(I,2), 'logical');
        return;
    end
    % Calculate mean and SD from non-background portion of the histogram
    xmean = 0.;
    for i = 1:numel(locs)
        xmean = xmean + locs(i) * counts(i);
    end
    xmean = xmean / pixcount;
    stdev = 0.;
    for i = 1:numel(locs)
        dif = xmean - locs(i);
        stdev = stdev + dif*dif*counts(i);
    end
    stdev = sqrt(stdev / pixcount);
    
    % Some weirdo images may produce thresholds way above the max pixel
    % value, so cap them at a level slightly less than the max.
    stdcut = round(xmean + stdev*cutoff);
    if maxbins == 256 && stdcut > 250
        stdcut = 250;
    elseif maxbins == 65536 && stdcut > 60000
        stdcut = 60000;
    end
    
    % Create mask image as 8-bit gray of the same size as I, set non-masked
    % pixels to 0, and masked ones, to 255 (max for 8-bit pixels).
    bitmask = I >= stdcut;
    S = zeros(imgH, imgW, 1, 'uint8');
    S(bitmask) = 255;
    % Resize mask image to 1/16, then binarize (0,255) it again
    S = imresize(S, [imgH/scaledown, imgW/scaledown]);
    bitmask = S > 250;
    S(bitmask) = 255;
    S(~bitmask) = 0;
    
    % WriteTiff(S, 'mask_before.tif', 8);

    % Make sure no 255-pixels at the image borders
    % (to prevent "index out of range" errors).
    sH = size(S,1);
    sW = size(S,2);
    for y = 1:sH
        S(y,1) = 0;
        S(y,sW) = 0;
    end
    for x = 1:sW
        S(1,x) = 0;
        S(sH,x) = 0;
    end

    % Now find all the masked areas; keep them if they are big enough
    % (> minarea), clear if they are too small.
    for y = 2:sH-1
        for x = 2:sW-1
            if S(y,x) == 255
                [yarr, xarr, sz] = FindArea(S, y, x, 255, 0);
                c = 0;
                if sz > minarea
                    c = 250;
                end
                for i = 1:sz
                    S(yarr(i), xarr(i)) = c;
                end
            end
        end
    end

    bitmask = S > 240;
    S(bitmask) = 255;
    
    % WriteTiff(S, 'mask_after.tif', 8);

    % Resize mask image back to the original size
    S = imresize(S, [imgH, imgW]);
    % And compute the final mask as any non-0 pixels
    bitmask = S > 1;
end
