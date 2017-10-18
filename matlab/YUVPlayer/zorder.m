function [ mat ] = zorder( size )
    if size <= 1
        mat = 0;
        return;
    end
    size = size / 4;
    mat = zorder(size);
    mat = [mat,mat+size;mat+2*size,mat+3*size];
    
end

