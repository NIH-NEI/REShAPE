function [dirlist] = ListDir(in_path, exts)
    dirlist = [];
    idx = 1;
    for iExt = 1:numel(exts)
        ext = exts{iExt};
        xlen = size(ext, 2);
        filt = fullfile(in_path, strcat('*', ext));
        d = dir(filt);
        for i = 1:numel(d)
            fn = d(i).name;
            basename = fn(1:end-xlen);
            dirlist(idx).filename = fn;
            dirlist(idx).pathname = fullfile(in_path,fn);
            dirlist(idx).basename = basename;
            dirlist(idx).ext = ext(2:end);
            idx = idx + 1;
        end
    end
end