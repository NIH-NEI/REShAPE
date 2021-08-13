function [imgobj] = ReadTiff(tiffn,tilepad,iscale)

% default value for maxTileArea: 400000000    
	maxTileArea = 400000000;
	if nargin > 2
		if iscale > 1.
			maxTileArea = maxTileArea / (iscale * iscale);
			disp(['maxTileArea = ' num2str(maxTileArea)]);
		end
	end

    I = imread(tiffn);
    imgH = size(I,1);
    imgW = size(I,2);
    
    nytiles = 1;
    nxtiles = 1;
    
    tileH = imgH;
    tileW = imgW;

% disp(['Tiff area: ' num2str((tileH+tilepad)*(tileW+tilepad))]);
    while (tileH+tilepad)*(tileW+tilepad) > maxTileArea
        if tileH > tileW
            nytiles = nytiles + 1;
            tileH = floor(imgH/nytiles);
        else
            nxtiles = nxtiles + 1;
            tileW = floor(imgW/nxtiles);
        end
    end
disp(['Num Tiles: ' num2str(nytiles*nxtiles)]);

    toth = 0;
    idx = 1;
    for yt = 1:nytiles
        y = (yt-1)*tileH;
        h = tileH;
        if yt==nytiles
            h = imgH - toth;
        end
        toth = toth + h;
        y = y + 1;

        if yt>1
            y = y - tilepad;
            h = h + tilepad;
        end
        if yt<nytiles
            h = h + tilepad;
        end
        totw = 0;
        for xt = 1:nxtiles
            x = (xt-1)*tileW;
            w = tileW;
            if xt==nxtiles
                w = imgW - totw;
            end
            totw = totw + w;
            x = x + 1;
            
            if xt>1
                x = x - tilepad;
                w = w + tilepad;
            end
            if xt<nxtiles
                w = w + tilepad;
            end
            res(idx).x = x;
            res(idx).y = y;
            res(idx).w = w;
            res(idx).h = h;
            res(idx).I = I(y:y+h-1, x:x+w-1);
            % bfGetPlane(r, 1, x, y, w, h);
            % disp(['[' num2str(toc) '] Finished reading tile ' num2str(idx) ' of ' num2str(numtiles)]);
            idx = idx + 1;
        end
    end

    imgobj.imageWidth = imgW;
    imgobj.imageHeight = imgH;
    imgobj.nytiles = nytiles;
    imgobj.nxtiles = nxtiles;
    imgobj.tiles = res;
end
