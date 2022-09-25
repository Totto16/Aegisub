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


#include "options.h"
#include "wakatime.h"
#include "frame_main.h"

#include "libaegisub/log.h"
#include "libaegisub/path.h"

#include <chrono>
#include <functional>
#include <string>
#include <ostream>
#include <sstream>
#include <iostream>

#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/utils.h>
#include <wx/process.h>
#include <wx/stream.h>
#include <wx/event.h>

namespace wakatime {

    wxString* StringArrayToString(wxArrayString* input, wxString* seperator){
            wxString* output = new wxString();
            for(size_t i=0; i < input->GetCount(); ++i){
                output->Append(i == 0 ? input->Item(i) :(wxString::Format("%s%s",*seperator,input->Item(i))));
            }
        return output;
    }

    #define BUF_SIZE 1024
    wxString* ReadInputStream(wxInputStream* input, bool trimLastNewLine = false){
        wxString* output = new wxString();
        void* buffer = malloc(BUF_SIZE);
        while(input->CanRead() && !input->Eof()){
            input->Read(buffer, BUF_SIZE);
            size_t read = input->LastRead();
            wxStreamError error = input->GetLastError();
            switch(error){
                case wxSTREAM_NO_ERROR:
                case wxSTREAM_EOF:
                    if(*((char*)buffer + strlen((char*)buffer)-1) == '\n' && trimLastNewLine){
                        read-=1;
                    }
                    output->append(wxString::FromUTF8((char*)buffer, read));
                    return output;
                case wxSTREAM_WRITE_ERROR:
                    LOG_E("wxInputStream WRITE ERROR");
                    output->Empty();
                    return output;
                case wxSTREAM_READ_ERROR:
                    LOG_E("wxInputStream READ ERROR");
                    output->Empty();
                    return output;
                default:
                    assert(false && "UNREACHABLE default in switch case");
            }
        }

        if(output->Last() == '\n' && trimLastNewLine){
            output = new wxString(output->ToAscii().data(),output->Length()-1 );
        }
        return output;
    }


    wxString* StringArrayToString(wxArrayString* input, const char * seperator = " "){
        return StringArrayToString(input, new wxString(seperator));
    }


    std::ostream& operator<<(std::ostream& os,  const wakatime::CLIResponse& arg){
        os << "CLIResponse: isOk: " << (arg.ok ? "yes" : "no" )<< "\n\tstreams:\n" <<
        "\t\terror: '" << (arg.error_string == nullptr ? "NULL" : arg.error_string->ToAscii()) << "'\n" <<
        "\t\toutput: '" << (arg.output_string == nullptr ? "NULL" : arg.output_string->ToAscii()) << "'\n\n"
        ;
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const wakatime::CLIResponse * arg){
        os << (*arg);
        return os;
    }



    using namespace std::chrono;


    plugin::plugin(){

        getSettings();

        if(!settings->enabled){
            return;
        }

        initialize();
    }

    void plugin::initialize(){
        setText(new wxString("Loading..."));

        cli = new wakatime::cli(this->settings);

        if(!cli->available){
            setText(new wxString("No wakatime CLI available"));
            return;
        }

        // TODO: move to the update / file set and detect if no file is set in the initializer of aegisub properly!
        setText(new wxString("No Project Selected"));
    }

    void plugin::getSettings(){
        if(settings == nullptr){
            OPT_SUB("Wakatime/API_Key", &plugin::getSettings, this);
            OPT_SUB("Wakatime/Enabled", &plugin::getSettings, this);
            OPT_SUB("Wakatime/Debug", &plugin::getSettings, this);

            settings = (Settings*)malloc(sizeof(Settings));
        }

        bool old_enabled = settings->enabled;

        settings->debug = OPT_GET("Wakatime/Debug")->GetBool();
        settings->enabled = OPT_GET("Wakatime/Enabled")->GetBool();
        std::string aegisub_key = OPT_GET("Wakatime/API_Key")->GetString();
        settings->key = new wxString(aegisub_key)

        if(old_enabled != settings->enabled){
            if(!old_enabled){
                initialize()
            }else{
                // this gets the string, that needs to be set in the title bar, since it's disabled it vanishes
                updateFunction();
            }
        }
    
        // TODO check validity of key, if it changed 
        // debug gets read when invoking cli, so maybe update these settings to, but since a pointer, it updates :)!

    }

    void plugin::changeProject(wxString* new_file, wxString* project_name){
        project_info.file_name = new_file;
        project_info.project_name = project_name;
		project_info.changed = true;

		sendHeartbeat(false);
	}

    void plugin::sendHeartbeat(bool is_write){

	    milliseconds now =  duration_cast<milliseconds>(steady_clock::now().time_since_epoch());

	    if (!(last_heartbeat + (2min) <= now || is_write || project_info.changed)){
		    return;
	    }

        project_info.changed = false;
	    last_heartbeat = now;

        if(settings->key->IsEmpty()){
            return;
        }

        wxArrayString *buffer = new wxArrayString();

        buffer->Add(wxString::Format("--language '%s'", plugin_info.short_type));
        buffer->Add(wxString::Format("--alternate-language '%s'", plugin_info.long_type));

        //TODO use translation for default file name!
        // subs_controller->Filename() return "Unbenannt" or "Untitled" when no file is loaded! (.ass has to be set additionally, and file path is ""!)

        buffer->Add(wxString::Format("--entity '%s'", project_info.file_name == nullptr ? "Unbenannt.ass": *project_info.file_name));
        // "--project" gets detected by the folder name! (the manual project name is also the folder name, atm at least!)
        buffer->Add(wxString::Format("--alternate-project '%s'", project_info.project_name == nullptr ?  "Unbenannt" : *project_info.project_name));

        if (is_write){
            buffer->Add("--write");
        }

        cli->invokeAsync(buffer, [this](CLIResponse response_string)->void{
            getTimeToday();
        });
	}

    void plugin::getTimeToday(){

        wxArrayString *buffer = new wxArrayString();
        buffer->Add("--today");
        cli->invokeAsync(buffer,[this](CLIResponse time_today)->void{
            if(time_today.ok){
                this->setText(time_today.output_string);
            }
        });
    }

    void plugin::setText(wxString *text){
        this->currentText = text;
        if(this->updateFunction != nullptr){
            this->updateFunction();
        }
    }



    //TODO make better
    cli::cli(Settings* _settings){

        this->settings = _settings;

        wxArrayString *output = new wxArrayString();

        //TODO get $HOME in a better way!!!
		long returnCode = wxExecute("bash  -c \"realpath ~",*output, wxEXEC_SYNC);

        if(returnCode != 0){
            available = false;
            status = ERROR;
            return;
        }

        // TODO: check if it's a director, thats also (theoretically possible, if it is the cli is in that directory!!!)
        // TODO download it if it's not possible, for that refer to some example implementation like e.g. wakatime vscode plugin!

        wxString* home_path = StringArrayToString(output);
		cli_path = new wxString(wxString::Format("%s/.wakatime/wakatime-cli",*home_path));
		
        if(!isPresent()){
            //TODO this is async, so how to handle that, what to do, if
            // it takes some time until it's activated, teh solution is to restart the plgin after the download, but then I have to see, how the project name getting will be done... 
			download();
		}

		available = true;


    }

    void cli::detectKey(){
        wxString* aegisub_key = settings->key;
        if(aegisub_key->IsEmpty()){
            wxArrayString *buffer = new wxArrayString();
            buffer->Add(wxString("--config-read api_key"));
            invokeAsync(buffer,[this](CLIResponse response)-> void{
        
                if (response.ok){
                    settings->key = response.output_string;
                    OPT_SET("Wakatime/API_Key")->SetString(settings->key->ToStdString());
                }
            });
        }else{
            wxArrayString *buffer = new wxArrayString();
            buffer->Add(wxString::Format("--config-write api_key=%s", *(settings->key)));
            invokeAsync(buffer,[this](CLIResponse response)-> void{
                if (!response.ok){
                    LOG_E("wakatime/execute/async") << "Couldn't save the wakatime key to the config!\n";
                }
            });
        }
    }




    bool cli::isPresent(){
		// TODO OS independent
        long returnCode = wxExecute(wxString::Format("bash  -c \"which '%s' > /dev/null\"",*cli_path), wxEXEC_SYNC);

        return returnCode == 0;
    }

    bool cli::download(){

        //TODO generalize (os independent) and finish, WIP
        return false;
        long returnCode = wxExecute(wxString::Format("bash  -c \"mkdir -p ~/.wakatime && wget  'https://github.com/wakatime/wakatime-cli/releases/download/v1.52.1-alpha.1/wakatime-cli-linux-amd64.zip -o ~/.wakatime/ > /dev/null && unzip && ln -S .... etc\"",*cli_path), wxEXEC_SYNC);

        return returnCode == 0;
    }




    void cli::invokeAsync(wxArrayString* options, std::function<void ( CLIResponse response)> callback){


        if(!available){
            CLIResponse response = {
                ok:false,
                error_string : new wxString("Wakatime CLI not installed"),
                output_string: nullptr
            };
            callback(response);
            return;
        }

        if(settings->debug){
            options->Add("--verbose");
        }

        if(settings->key != nullptr && !settings->key->IsEmpty()){
            options->Add(wxString::Format("--key '%s'",*(settings->key)));
        }
        options->Add(wxString::Format("--plugin 'aegisub/%s %s/%s'",plugin_info.plugin_version, plugin_info.plugin_name, *(plugin_info.aegisub_version)));

        wxString command = wxString::Format("%s %s", *cli_path, * StringArrayToString(options));
        wxProcess* process = new wxProcess();
        process->Redirect();


        std::function< void (wxProcessEvent& event)> lambda = ([process, command, callback](wxProcessEvent& event)-> void{

            CLIResponse response = {
                ok:true,
                error_string : nullptr,
                output_string: nullptr
            };
            
            wxInputStream* outputStream = process->GetInputStream() ;
            if(outputStream == nullptr){
                LOG_E("wakatime/execute/async") << "Command couldn't be executed: " << command.ToAscii();
                response.error_string = new wxString("Stdout was NULL");
                response.ok = false;
                callback(response);
                return;
            }
            
            wxInputStream* errorStream = process->GetErrorStream() ;
            if(errorStream == nullptr){
                LOG_E("wakatime/execute/async") << "Command couldn't be executed: " << command.ToAscii();
                response.error_string = new wxString("Stderr was NULL");
                response.ok = false;
                callback(response);
                return;
            }


            wxString* error_string = ReadInputStream(errorStream, true);
            wxString* output_string = ReadInputStream(outputStream, true);

            


            if(event.GetExitCode() != 0 || output_string == nullptr || !error_string->IsEmpty()){
                LOG_E("wakatime/execute/async") << "Command couldn't be executed: " << command.ToAscii();

                LOG_E("wakatime/execute/async") << "The Errors were: " << error_string->ToAscii();
                
                response.error_string = error_string;
                response.ok = false;
                callback(response);
                return;
            } 


            response.output_string = output_string;


            callback(response);
            return;
        });

        process->Bind(wxEVT_END_PROCESS, lambda);


        long pid = wxExecute(command, wxEXEC_ASYNC,process);

        if(pid == 0){
            LOG_E("wakatime/execute") << "Command couldn't be executed: " << command.ToAscii();
            return;
        }
    }


	wakatime::plugin *wakatime_plugin_instance = nullptr;
	void init() {
		wakatime_plugin_instance = new plugin();
	}

	void clear() {
		delete wakatime_plugin_instance;
	}

    void setUpdateFunction(std::function<void ()> updateFunction){
        updateFunction();
        if(wakatime_plugin_instance == nullptr){
            LOG_E("plugin/wakatime") << "ERROR: couldn't set the update function!\n";
            return;
        }
        wakatime_plugin_instance->updateFunction = updateFunction;
    }

    wxString getText(){
        if(wakatime_plugin_instance->currentText == nullptr){
            return wxString("");
        }
        return wxString::Format(" - %s", *(wakatime_plugin_instance->currentText));
    }


	void update(bool is_write, agi::fs::path const* filename){
        if(filename != nullptr){
            wxString* temp_file_name = new wxString(filename->string());
            wxString* temp_project_name = new wxString(filename->parent_path().filename().string());
            wakatime_cli->project_info.changed =  wakatime_cli->project_info.file_name == nullptr || !wakatime_cli->project_info.file_name->IsSameAs(*temp_file_name)
                || wakatime_cli->project_info.project_name == nullptr || wakatime_cli->project_info.project_name->IsSameAs(*temp_project_name);

            if(wakatime_cli->project_info.changed){
                wakatime_cli->project_info.file_name = temp_file_name;
                wakatime_cli->project_info.project_name = temp_project_name; 
            }
        }
	    wakatime_cli->send_heartbeat(is_write);
	}

} // namespace wakatime



