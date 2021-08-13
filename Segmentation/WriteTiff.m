function WriteTiff(S,image_path,bitsps)
    if bitsps==8
        bps = 1;
    elseif bitsps==16
        bps = 2;
    else
        error(['Invalid bitsps parameter (' num2str(bitsps) ') - must be either 8 or 16.']);
    end
    megs = size(S,1)*size(S,2)*bps/(1024.*1024.);
    f = 'w';
    compr = Tiff.Compression.LZW;
    if megs>2040.
        f = 'w8';
        compr = Tiff.Compression.None;
    end
    t = Tiff(image_path,f);
    t.setTag('Photometric',Tiff.Photometric.MinIsBlack);
    t.setTag('Compression',compr);
    t.setTag('BitsPerSample',bitsps);
    t.setTag('SamplesPerPixel',size(S,3));
    t.setTag('SampleFormat',Tiff.SampleFormat.UInt);
    t.setTag('ImageLength',size(S,1));
    t.setTag('ImageWidth',size(S,2));
    t.setTag('PlanarConfiguration',Tiff.PlanarConfiguration.Chunky);

    t.write(S);
    t.close();
end