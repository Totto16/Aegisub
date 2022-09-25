// Copyright (c) 2022, Totto
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#pragma once

#include "libaegisub/path.h"
#include "options.h"
#include "version.h"
#include "frame_main.h"

#include <chrono>
#include <functional>

#include <wx/string.h>
#include <wx/arrstr.h>



namespace wakatime {


void init();

void clear();

void update(bool is_write, agi::fs::path const* filename = nullptr);

void setUpdateFunction(std::function<void ()> updateFunction);

wxString getText();


using namespace std::chrono;

    typedef struct  {
        const wxString program;
        const wxString plugin_name;
        const wxString short_type;
        const wxString long_type;
        const wxString plugin_version;
        wxString* aegisub_version;
    } PluginInfo;

    typedef struct  {
        bool ok;
        wxString* error_string; // is nullptr if no error occurred
        wxString* output_string; // can be empty and is nullptr if an error occured
    } CLIResponse;


    std::ostream& operator<<(std::ostream& os,  const wakatime::CLIResponse& arg);

    std::ostream& operator<<(std::ostream& os, const wakatime::CLIResponse * arg);

    typedef struct {
        wxString* project_name;
        wxString* file_name;
        bool changed;
    } ProjectInfo;

    typedef struct {
        bool enabled;
        bool debug;
        wxString* key;
    } Settings;


    typedef enum {
        CHECKING,
        UP_TO_DATE,
        OUTDATED,
        UPDATING,
        NOT_INSTALLED,
        INSTALLING,
        ERROR
    } CLIStatus;


    //TODO link to the wakatime docs for creating a plugin!

    /// @class plugin
    /// the wakatime plugin class, it has the needed methods to send detect and display everything, 
    /// it passes the heartbeats to the cli class 
    class plugin {
    public:
        plugin();

        void changeProject(wxString* new_file, wxString* project_name);

        wxString* currentText = nullptr;
        std::function<void ()> updateFunction;

    private:
        ProjectInfo project_info = {
            project_name : nullptr,
            file_name: nullptr,
            changed: false
        };
        PluginInfo plugin_info = {
            program: "Aegisub",
            plugin_name: "aegisub-wakatime",
            short_type: "ASS",
            long_type:"Advanced SubStation Alpha",
            plugin_version: "1.0.1",
            aegisub_version: new wxString(GetAegisubLongVersionString())
        };

        
        Settings *settings = nullptr; 
        milliseconds last_heartbeat = 0ms;
        cli *cli = nullptr;

        void initialize();
        void getSettings();
        void getTimeToday();
        void setText(wxString* text);
    };


    /// @class cli
    /// the wakatime cli class, it has the needed methods to work with the cli, it detects it, downloads it if necessary 
    /// and performs heartbeats
    class cli {
    public:
        cli(Settings *settings, PluginInfo* plugin_info);

        void invokeAsync(wxArrayString* options, std::function<void ( CLIResponse response)> callback);
        void sendHeartbeat(bool is_write);

        bool available;

    private:
        wxString* cli_path = nullptr;
        CLIStatus status = CHECKING;
        Settings * settings;

        void detectKey();
        bool isPresent();
        bool download();


        
    };
}
