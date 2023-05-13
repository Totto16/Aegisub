local record __lfs__impl
    type CPointer = integer
    type DirectoryIterator = CPointer

    -- these isn't correct. since multiüple return values are realized in *to return type as argument, but since it's only used internally i's not that bad to use incorrects mappings, maybe we should rewrite these to use integer as pointer, but then we lose information
    chdir: function(dir: string): boolean, string
    currentdir: function(): string, string
    mkdir: function(dir: string): boolean, string
    rmdir: function(dir: string): boolean, string
    touch: function(path: string): boolean, string
    get_mtime: function(path: string): integer, string
    get_mode: function(path: string): string, string
    get_size: function(path: string): integer, string
    dir_new: function(path: string): DirectoryIterator, string
    dir_free: function(it: DirectoryIterator)
    dir_next: function(it: DirectoryIterator): string, string
    dir_close: function(it: DirectoryIterator)
end

return __lfs__impl
