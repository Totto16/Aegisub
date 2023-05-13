local type argcheck = function<T>(argmtfmt: string): function(fn: function(...: any) : T): T | nil

return argcheck
