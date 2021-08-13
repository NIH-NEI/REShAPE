function [imdb] = format_data(raw_input_dir,seg_input_dir,varargin)
%GENERATE_DATA Summary of this function goes here
%   Detailed explanation goes here

    %% Parse inputs and determine if data has been previously formatted
    [out_dir,img_size,lbl_size,file_type,holdout,weight] = parse_inputs(varargin);
    
    if sum(lbl_size>img_size)>0
        error('Label size must be smaller than image size.')
    end
    
    if exist(fullfile(out_dir,'imdb.mat'),'file')
        
        disp('Data already exists. Loading and checking data...')
        load(fullfile(out_dir,'imdb.mat'));
        if (isequal(imdb.meta.imSize,img_size) && ...
            isequal(imdb.meta.holdout,holdout))
            disp('Data is formatted properly');
            return
        else
            disp('Data is not formatted properly, reformatting data...')
        end
    else
        disp('Formatting data...')
    end
    
    %% Initialize the image database
    imdb = struct();
    
    imdb.meta.imSize = img_size; % size of each image
    imdb.meta.holdout = holdout; % percentage of data held out for validation
    imdb.meta.fileType = file_type; % file extension of original images
    imdb.meta.mean = [];
    imdb.meta.std = [];
    imdb.meta.rawImage = {}; % name of raw images
    imdb.meta.segImage = {}; % name of segmented images
    
    imdb.images.label = single([]); % label for segment
    imdb.images.data = single([]); % images
    imdb.images.set = []; % 1 if image is for training, 2 if image is for validation
    imdb.images.id = []; % numerical id for image
    imdb.images.imgId = []; % index of images this segment was taken from

    %% Check if directories are valid, display file alignment
    if ~exist(raw_input_dir)
        error('Invalid raw image input directory.')
    elseif ~exist(seg_input_dir)
        error('Invalid segmented image input directory.')
    end
    
    raw_images = dir(fullfile(raw_input_dir,file_type));
    seg_images = dir(fullfile(seg_input_dir,file_type));
    
    disp('-----------------------------------------------')
    disp(' File Alignment')
    disp('-----------------------------------------------')
    for i = 1:length(raw_images)
    disp(['Raw Image ' num2str(i) ': ' raw_images(i).name])
    disp(['Seg Image ' num2str(i) ': ' seg_images(i).name])
    disp(' ')
    end
    disp('-----------------------------------------------')
    
    %% Load files and format data
    disp(' Loading and Formating')
    disp('-----------------------------------------------')
    for i = 1:length(raw_images)
        % Load raw and segmented images
        disp(['Raw Image ' num2str(i) ': ' raw_images(i).name])
        raw_img = single(imread(fullfile(raw_input_dir,raw_images(i).name)));
        disp(['Seg Image ' num2str(i) ': ' seg_images(i).name])
        seg_img = imread(fullfile(seg_input_dir,seg_images(i).name));
        if ~isequal(size(raw_img),size(seg_img))
            disp('Raw and segmented images are not the same size. Moving to next image.')
            disp('')
            continue
        end
        disp(' ');
        imdb.meta.rawImage(end+1) = {raw_images(i).name};
        imdb.meta.segImage(end+1) = {seg_images(i).name};
        imdb.meta.mean(end+1) = mean(raw_img(raw_img>0));
        imdb.meta.std(end+1) = std(raw_img(raw_img>0));
        
        % Create weights based on distance transforms
        seg_img = single(seg_img>0);
        seg_img_pos = ones(size(seg_img));
        %seg_img_neg = ones(size(seg_img));
        seg_img_neg = (1./bwdist(seg_img>0)).^weight;
        seg_img(seg_img>0) = seg_img_pos(seg_img>0);
        seg_img(seg_img==0) = -seg_img_neg(seg_img==0);
        
        % Alter weights based on pixel intensity
        pos_mean = imboxfilt(raw_img.*single(seg_img>0),127,'NormalizationFactor',1);
        pos_mean2 = imboxfilt(raw_img.^2.*single(seg_img>0),127,'NormalizationFactor',1);
        pos_sum = imboxfilt(single(seg_img>0),127,'NormalizationFactor',1);
        pos_mean = pos_mean./pos_sum;
        pos_mean2 = pos_mean2./pos_sum;
        pos_std = sqrt(pos_mean2 - pos_mean.^2);
        
        pos_norm = -((raw_img - pos_mean)./pos_std);
        pos_norm = ((pos_norm>0).*pos_norm).^3;
        seg_img(seg_img>0) = seg_img(seg_img>0) + pos_norm(seg_img>0);
        
        %neg_norm = ((raw_img - im_mean)./im_std);
        %neg_norm = ((neg_norm>0).*neg_norm).^2;
        %seg_img(seg_img<0) = seg_img(seg_img<0) - neg_norm(seg_img<0);
        
        % Generate indices
        [nRows, nCols] = size(raw_img);
        [cindex,rindex] = meshgrid(0:img_size(2)-1,0:img_size(1)-1);
        img_ind = cindex(:)*nRows+rindex(:);
        seg_offsets = (img_size-lbl_size)/2;
        [cindex,rindex] = meshgrid(seg_offsets(2):img_size(2)-seg_offsets(2)-1,...
                                   seg_offsets(1):img_size(1)-seg_offsets(1)-1);
        seg_ind = cindex(:)*nRows+rindex(:);
        
        [cindex,rindex] = meshgrid(0:200:nCols-img_size(2),...
                                   1:200:nRows-img_size(1)+1); %for image data
        raw_indices = nRows*cindex(:)+rindex(:);
        img_indices = bsxfun(@plus,img_ind(:),raw_indices(:)');
        seg_indices = bsxfun(@plus,seg_ind(:),raw_indices(:)');
        
        % Get pixels and reshape into images
        img_pixels = raw_img(img_indices(:));
        img_pixels = reshape(img_pixels,img_size(1),img_size(2),1,[]);
        seg_pixels = seg_img(seg_indices(:));
        seg_pixels = reshape(seg_pixels,lbl_size(1),lbl_size(2),1,[]);
        num_images = size(img_pixels,4);
        
        % Add images to image database
        imdb.images.data = cat(4,imdb.images.data,img_pixels);
        imdb.images.label = single(cat(4,imdb.images.label, seg_pixels));
        imdb.images.imgId = [imdb.images.imgId; i*ones(num_images,1)];
        imdb.images.id = [imdb.images.id; length(imdb.images.id) + [1:num_images]'];
    end
    
    disp(['Formatted ' num2str(size(imdb.images.data,4)) ' images!']);
    if isempty(imdb.images.data)
        return
    end
    
    %% Randomize data, subtract mean, assign sets, and save
    cellSum = sum(imdb.images.label(imdb.images.label>0));
    backSum = -sum(imdb.images.label(imdb.images.label<0));
    backCellRatio = backSum/cellSum;
    imdb.images.label(imdb.images.label>0) = backCellRatio.*imdb.images.label(imdb.images.label>0);
    imdb.images.label = single(imdb.images.label);
    imdb.images.imgId = single(imdb.images.imgId);
    imdb.images.id = single(imdb.images.id);
    
    % Reset the random number generator
    rng(1);
    setIndex = randsample(size(imdb.images.label,4),round(size(imdb.images.label,4)*imdb.meta.holdout));
    imdb.images.set = single(ones(size(imdb.images.label,4),1));
    imdb.images.set(setIndex) = 2;
    
    if ~exist(out_dir,'dir')
        mkdir(out_dir);
    end
    save(fullfile(out_dir,'imdb.mat'),'imdb','-v7.3')

    %% Function to parse inputs
    function [out_dir,img_size,lbl_size,file_type,holdout,weight] = parse_inputs(inputs)
        out_dir = fullfile(pwd,'Data');
        file_type = '*.tif';
        img_size = [256 256];
        holdout = 0.7;
        lbl_size = [1 1];
        weight = 0;
        
        if isempty(inputs)
            return
        end
        
        for input = 1:2:length(inputs)
            switch inputs{input}
                case 'lblSize'
                    lbl_size = inputs{input+1};
                case 'outDir'
                    out_dir = inputs{input+1};
                case 'imSize'
                    img_size = inputs{input+1};
                case 'fileType'
                    file_type = inputs{input+1};
                    if strcmp(file_type(1),'.')
                        file_type = ['*' file_type];
                    elseif ~strcmp(file_type(1:2),'*.')
                        file_type = ['*.' file_type];
                    end
                case 'holdout'
                    holdout = inputs{input+1};
                case 'weight'
                    weight = inputs{input+1};
            end
        end
    end

end

