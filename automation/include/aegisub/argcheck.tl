local assert = assert
local error = error
local select = select
local tostring = tostring
local type = type
local record TableInfo
  record ClassInfo
    __name: string
  end

    __class: ClassInfo
  end

local is_type = function<A>(v: A, ty: string, expected: string): boolean
  return ty == expected or (ty == 'table' and (v as TableInfo).__class and (v as TableInfo).__class.__name == expected)
end
-- if teals supports these generic stuff better: use this: return function<T>(argfmt : string): function(fn: function(...: any) : T): function(...:any):  T | nil
return function(argfmt : string): function(fn: function(...: any) : any): function(...:any):  any | nil
  assert(type(argfmt) == 'string')
  local min_args = 0
  local max_args = 0
  local checks : { integer: function(i: integer, v: any | nil): function() }= { }
  for arg in argfmt:gmatch('[^ ]+') do
    if arg == '...' then
      max_args = nil
      break
    end
    max_args = max_args + 1
    local optional = arg:sub(1, 1) == '?'
    if optional then
      arg = arg:sub(2)
    else
      min_args = min_args + 1
    end
    if arg:find('|') then
      local types: {integer: string}
      do
        local _accum_0 = { }
        local _len_0 = 1
        for ty in arg:gmatch('[^|]+') do
          _accum_0[_len_0] = ty
          _len_0 = _len_0 + 1
        end
        types = _accum_0
      end
      checks[max_args] = function(i: integer, v: any | nil): function()
        if v == nil then
          if optional then
            return 
          end
          error("Argument #" .. tostring(i) .. " should be a " .. tostring(arg) .. ", is nil", 4)
        end
        local ty = type(v)
        for _index_0 = 1, #types do
          local argtype = types[_index_0]
          if is_type(v, ty, argtype) then
            return 
          end
        end
        return error("Argument #" .. tostring(i) .. " should be a " .. tostring(arg) .. ", is " .. tostring(ty) .. " (" .. tostring(v) .. ")", 3)
      end
    else
      checks[max_args] = function(i: integer, v: any | nil): function()
        if v == nil then
          if optional then
            return 
          end
          error("Argument #" .. tostring(i) .. " should be a " .. tostring(arg) .. ", is nil", 4)
        end
        local ty = type(v)
        if is_type(v, ty, arg) then
          return 
        end
        return error("Argument #" .. tostring(i) .. " should be a " .. tostring(arg) .. ", is " .. tostring(ty) .. " (" .. tostring(v) .. ")", 3)
      end
    end
  end

  return function<T>(fn: function(...: any) : T): function(...: any):  T | nil
    return function(...: any): T
      local arg_count = select('#', ...)
      if arg_count < min_args or (max_args and arg_count > max_args) then
        if min_args == max_args then
          error("Expected " .. tostring(min_args) .. " arguments, got " .. tostring(arg_count), 3)
        else
          if max_args then
            error("Expected " .. tostring(min_args) .. "-" .. tostring(max_args) .. " arguments, got " .. tostring(arg_count), 3)
          else
            error("Expected at least " .. tostring(min_args) .. " arguments, got " .. tostring(arg_count), 3)
          end
        end
      end
      for i = 1, arg_count do
        if not checks[i] then
          break
        end
        checks[i](i, select(i, ...))
      end
      return fn(...)
    end
  end
end
