-- Global defintions for functions according to .cpp files

-- TODO: these need to be reset (since we change them in the gobal env), even if teal sets them as constants!
-- global constdofile : nil
-- global loadfile : nil
-- global include : Include


global filename : string

global record aegisub
    record clipboard_impl 
        get: function(): string
        set: function(str: string): boolean
    end

    register_macro: function()
    register_filter: function()
    text_extents: function()
	frame_from_ms: function()
	ms_from_frame: function()
	video_size: function()
	keyframes: function()
	decode_path: function()
	cancel: function()
	lua_automation_version: integer
    __init_clipboard: function(): clipboard_impl
	file_name: function()
	gettext: function()
	project_properties: function()
	get_audio_selection: function()
	set_status_text: function()
end





--TODO document somewhere, that it needs:
-- global version : integer
-- global script_name : string
-- global script_description : string
-- global script_author : string
-- global script_version : string
