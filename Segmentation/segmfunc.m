function [params] = segmfunc(jsonpar)
    params = jsondecode(fileread(jsonpar));

    disp(['isdeployed = ' num2str(isdeployed)]);
    if isdeployed
        disp(['ctfroot = ' ctfroot]);
        mlog = fopen(params.logfile, 'w');
        params.cwd = strcat(ctfroot, '/segmfunc');
    end
	
    arti_filter = 0.;
    if isfield(params,'arti_filter')
        arti_filter = params.arti_filter;
    end
    
    if isfield(params, 'cziOptions')
        czi_options = params.cziOptions;
    else
        czi_options.save_source = 0;
        czi_options.planes(1).plane = 1;
        czi_options.planes(1).suff = 'main';
    end
    
    save_source = czi_options.save_source;
    save_mask = 0;      % this is a debug option; normally set to 0
    
    if save_source
        source_dir = czi_options.source_dir;
        if ~exist(source_dir, 'dir')
            mkdir(source_dir);
        end
        disp(['Tile Source Directory: ' source_dir]);
    end
    
    if (save_mask > 0) && (arti_filter > 0.)
        msk_dir = fullfile(params.tgtDir, 'Mask');
        if ~exist(msk_dir, 'dir')
            mkdir(msk_dir);
        end
        disp(['Tile Artifact Mask Directory: ' msk_dir]);
    end
	
	% options_path = 'options.mat';
	network_path = fullfile(params.cwd,'cnn.mat');

	run(fullfile(params.cwd, 'matconvnet', 'matlab', 'vl_setupnn'));
	addpath(fullfile(params.cwd, 'matconvnet', 'examples'));
	
	try
	    if params.useGpu
	        I = gpuArray(single(1));
	    else
	        I = single(1);
	    end
	    vl_nnconv(I,I,[]);
	    disp(['MatConvNet is compiled.']);
	catch
	    if isdeployed
		disp(["Skip compiling MatConvNet"]);
	    else
		tic;
		disp(['MatConvNet is not compiled, trying to compile now...']);
		vl_compilenn(params.mex);
		disp(['MatConvNet compiled in: ' num2str(toc)]);
	    end
	end

	%% Prepare paths
    in_list = ListDir(params.srcDir, {'.tif', '.tiff', '.czi'});
    image_paths = [];
    j = 1;
    for i = 1:numel(in_list)
        im_name = in_list(i).filename;
        in_pathname = in_list(i).pathname;
        out_pathname = fullfile(params.tgtDir, strcat(in_list(i).basename, '.json'));
	    if exist(out_pathname,'file')
	        disp(['Already processed image: ' im_name]);
	        if isdeployed
	        	fprintf(mlog, 'Already processed image: %s\n', im_name);
	        end
            disp(['Writing metadata <' im_name '> -> ' out_pathname]);
	        if isdeployed
            	fprintf(mlog, 'Writing metadata <%s> -> %s\n', im_name, out_pathname);
            end
	        continue;
        end
        image_paths(j).srcname = in_list(i).filename;
        image_paths(j).srcfile = in_pathname;
        image_paths(j).tgtfile = out_pathname;
        image_paths(j).basename = in_list(i).basename;
        image_paths(j).ext = in_list(i).ext;
        j = j + 1;
    end
    
	num_images = numel(image_paths);
	disp(['Found ' num2str(num_images) ' image(s) in directory: ' params.srcDir]);
	if isdeployed
    	fprintf(mlog, 'Found %d image(s) in directory: %s\n', num_images, params.srcDir);
    end
	
	%% Load network
	% addpath(fullfile(params.cwd, 'src'));
	% load(options_path);
	
	prep = struct('imgSize', [256 256], 'lblSize', [200 200], ...
		'normImage', 1, 'windowSize', 127, 'maxNorm', 6);
	opts = struct('network', struct('type', 6), 'prep', prep);
	
	load(network_path);
	
	%% Run network on images
	for image = 1:num_images
        im_name = image_paths(image).srcname;
        im_path = image_paths(image).srcfile;
        json_path = image_paths(image).tgtfile;
        ext = image_paths(image).ext;
        basename = image_paths(image).basename;
        
        tic;
        disp(['Segmenting image: ' im_name ' <- ' im_path]);
        if isdeployed
        	fprintf(mlog, 'Segmenting image: %s <- %s\n', im_name, im_path);
        end
        if strcmp(ext, 'czi')
            czi_options.basename = basename;
            imgobj = ReadCzi(im_path, params.tilepad, params.iscale, czi_options);
            tgimgobj.czi_planes = czi_options.planes;
        else
            imgobj = ReadTiff(im_path, params.tilepad, params.iscale);
        end
        disp(['Loaded image ' im_name ' in: ' num2str(toc)]);
        if isdeployed
        	fprintf(mlog, 'Loaded image %s in: %s\n', im_name, toc);
        end

        tgtiles = [];
        numrows = imgobj.nytiles;
        numcols = imgobj.nxtiles;
        numtiles = numrows * numcols;
        idx = 1;
        for row = 1:numrows
            for col = 1:numcols
                disp(['Processing tile ' num2str(idx) ' of ' num2str(numtiles)]);
                if isdeployed
                	fprintf(mlog, 'Processing tile %d of %d\n', idx, numtiles);
                end
                tile = imgobj.tiles(idx);
                if isfield(tile, 'source_path')
                	I = imread(tile.source_path);
                else
                	I = tile.I;
                end
                orig_h = size(I,1);
                orig_w = size(I,2);
                if params.iscale ~= 1.
                	I = imresize(I, params.iscale);
                end
                if arti_filter > 0.
                    arti_mask = ArtiMask(I, arti_filter);
                    I(arti_mask) = 0;
                    if save_mask
                        msk_name = strcat(basename, '_tile', num2str(idx), '_mask.tif');
                        msk_path = fullfile(msk_dir, msk_name);
                        S = zeros(size(I,1),size(I,2),1,'uint8');
                        S(arti_mask) = 255;
                        WriteTiff(S, msk_path, 8);
                    end
                end
                S = zeros(size(I,1),size(I,2),1,'uint8');

                if opts.network.type==6
                    S = CNNsegment(I, net, opts, params.batchSize, params.useGpu);
                elseif opts.network.type==4
                    S = CNNpixelclass(I, net, opts, params.batchSize, params.useGpu);
                end;
                blackPixels = S == 1;
                S(blackPixels) = 255;
                if params.iscale ~= 1.
                    S = imresize(S, [orig_h, orig_w]);
                    blackPixels = S > 127;
                    S(blackPixels) = 255;
                    S(~blackPixels) = 0;
                end   
                
                out_name = strcat(basename, '_tile', num2str(idx), '.tif');
                out_path = fullfile(params.tgtDir, out_name);
                
                disp(['Writing segmented image <' im_name '> tile ' num2str(idx) ' -> ' out_path]);
                if isdeployed
                	fprintf(mlog, 'Writing segmented image <%s> tile %d -> %s\n', im_name, idx, out_path);
                end
                WriteTiff(uint8(S),out_path,8);
                
                tgtiles(idx).name = out_name;
                tgtiles(idx).x = tile.x - 1;
                tgtiles(idx).y = tile.y - 1;
                tgtiles(idx).w = tile.w;
                tgtiles(idx).h = tile.h;
                if isfield(tile, 'source')
                	tgtiles(idx).source = tile.source;
                end
                if isfield(tile, 'xsources')
                    tgtiles(idx).xsources = tile.xsources;
                end
                %if save_source
                %    out_name = strcat(basename, '_source', num2str(idx), '.tif');
                %    out_path = fullfile(source_dir, out_name);
                %    disp(['Writing tile source <' im_name '> tile ' num2str(idx) ' -> ' out_path]);
                %    WriteTiff(I,out_path,16);
                %    tgtiles(idx).source = out_path;
                %end
                
                idx = idx + 1;
            end
        end
        
        tgimgobj.source = im_path;
        tgimgobj.width = imgobj.imageWidth;
        tgimgobj.height = imgobj.imageHeight;
        tgimgobj.numtilerows = numrows;
        tgimgobj.numtilecolumns = numcols;
        tgimgobj.tiles = tgtiles;
        
        jsonStr = jsonencode(tgimgobj);
        fid = fopen(json_path, 'w');
        if fid == -1, error('Cannot create JSON file'); end
        disp(['Writing metadata <' im_name '> -> ' json_path]);
        if isdeployed
        	fprintf(mlog, 'Writing metadata <%s> -> %s\n', im_name, json_path);
        end
        fwrite(fid, jsonStr, 'char');
        fclose(fid);
	    
	    disp(['Finished processing image ' im_name ' in: ' num2str(toc)]);
	    if isdeployed
	    	fprintf(mlog, 'Finished processing image %s in: %d\n', im_name, toc);
	    end
	end
end
