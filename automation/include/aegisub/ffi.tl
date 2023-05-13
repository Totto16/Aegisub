local ffi = require('ffi')
ffi.cdef([[void free(void *ptr);
]])
local char_ptr = ffi.typeof('char *')
local string_impl = function(cdata : ffi.CData): nil | string
  if cdata == nil then
    return nil
  end
  local str = ffi.string(cdata)
  if type(cdata) == char_ptr then
    (ffi.C as ffi.CDefinedSymbols).free(cdata)
  end
  return str
end



local err_buff : {string} = ffi.new('char *[1]') as {string}
local err_arg_to_multiple_return = function(f: function(arg: any, err_buff :{string}|nil): any): function(arg: any):any 
  return function(arg : any): any | nil, nil | string 
    err_buff[0] = nil
    local result : any
    if arg ~= nil then
      result = f(arg, err_buff)
    else
      result = f(err_buff)
    end
    local errmsg = string_impl(err_buff[0])
    if errmsg then
      return nil, errmsg
    end
    return result
  end
end

return {
  string = string_impl,
  err_arg_to_multiple_return = err_arg_to_multiple_return
}
