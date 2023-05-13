
local record lfs
  enum FieldType
    "mode"
    "modification"
    "size"
    "west"
  end  

  record AllFields 
      mode: string
      modification: integer
      size: integer
  end

  record Dir 
    -- TODO: this is a complicated record with many __methods etc.
  end

  attributes: function(path: string, field:  FieldType | nil): string | integer | nil | AllFields
  chdir: function(dir: string): boolean, string
  currentdir: function(): string, string
  dir: function(path: string): string | nil, Dir
  mkdir: function(dir: string): boolean, string
  rmdir: function(dir: string): boolean, string
  touch: function(path: string): boolean, string
end


return lfs
