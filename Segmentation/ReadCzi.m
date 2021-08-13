function [imgobj] = ReadCzi(czifn,tilepad,iscale,czi_options)
    if ~isdeployed
        addpath(fullfile('.','bfmatlab'));
    end

% default value for maxTileArea: 350000000    
	maxTileArea = 350000000;
	if nargin > 2
		if iscale > 1.
			maxTileArea = maxTileArea / (iscale * iscale);
		end
    end
    save_source = 0;
    if nargin > 3
        save_source = czi_options.save_source;
    end

    tic;
    r = bfGetReader(czifn);
    imgW = r.getSizeX();
    imgH = r.getSizeY();
    % disp(['Width: ' num2str(imgW) ' Height: ' num2str(imgH)]);

    nytiles = 1;
    nxtiles = 1;
    
    tileH = imgH;
    tileW = imgW;

% disp(['SZI area: ' num2str((tileH+tilepad)*(tileW+tilepad))]);
% 350000000    
    while (tileH+tilepad)*(tileW+tilepad) > maxTileArea
        if tileH > tileW
            nytiles = nytiles + 1;
            tileH = floor(imgH/nytiles);
        else
            nxtiles = nxtiles + 1;
            tileW = floor(imgW/nxtiles);
        end
    end
    
    numtiles = nxtiles * nytiles;
disp(['Num Tiles: ' num2str(numtiles)]);
    
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
            if save_source
                for iplane = 1:numel(czi_options.planes)
                    plane = czi_options.planes(iplane);
                    p = plane.plane;
                    out_name = strcat(czi_options.basename, '_p', num2str(p), '_', plane.suff, '_tile', num2str(idx), '.tif');
                    out_path = fullfile(czi_options.source_dir, out_name);
                    out_rel = strcat(czi_options.source_rel, '/', out_name);
                    I = bfGetPlane(r, p, x, y, w, h);
                    WriteTiff(I, out_path, 16);
                    if iplane == 1
                        res(idx).plane = p;
                        res(idx).source = out_rel;
                        res(idx).source_path = out_path;
                    else
                        res(idx).xsources(iplane-1).plane = p;
                        res(idx).xsources(iplane-1).name = out_rel;
                    end
                end
            else
                res(idx).I = bfGetPlane(r, 1, x, y, w, h);
            end
            disp(['[' num2str(toc) '] Finished reading tile ' num2str(idx) ' of ' num2str(numtiles)]);
            idx = idx + 1;
        end
    end

    imgobj.tiles = res;
    imgobj.imageWidth = imgW;
    imgobj.imageHeight = imgH;
    imgobj.nxtiles = nxtiles;
    imgobj.nytiles = nytiles;
end
