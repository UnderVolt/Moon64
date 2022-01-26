#include <iostream>
#include "types.h"
#include "utils.h"
#include "sm64.hpp"
#include "json.hpp"
#include "extractor/extractor.h"

extern "C" {
#include "extractor/n64/aiff_extract_codebook.h"
#include "extractor/n64/adpcm/vadpcm.h"
}

using namespace std;
namespace sm64 = MoonUtils;

#define STR(x) #x

string main_folder = "assets";
vector<string> defaults;

ExtractRule rules[] = {
    { "graphics", { "actors", "levels", "textures" }},
    { "/",        { "sound", "fonts", "langs" }}
};

Transformer transformers[] = {
    { "aiff", [](string filename, string fullpath) {
        string dir   = sm64::dirname(fullpath);
        string aiff  = fullpath;
        string aifc  = sm64::join(dir, filename + ".aifc");
        string table = sm64::join(dir, filename + ".table");
        string codebook = string(aiff_extract_codebook(CNV(aiff)));
        sm64::writeFile(table, codebook);
        vadpcm_enc(CNV(table), CNV(aiff), CNV(aifc));
    }}
};

void remove_unused(string dir){
    sm64::rm(sm64::join(sm64::join(dir, main_folder), "demos"));
}

void copy_assets(string dir){
    for (auto& rule : rules){
        string parent = sm64::join(sm64::join(dir, main_folder), rule.name);
        if(!sm64::exists(parent))
            sm64::mkdir(parent);

        for (auto& folder : rule.rules){
            string from = sm64::join(dir, folder);
            string to   = sm64::join(parent, folder);
            sm64::move(from, to);
        }
    }
}

void copy_defaults(string dir) {
    if(defaults.empty()) return;
    cout << "Copying defaults" << endl;
    for(auto& path : defaults){
        sm64::copy(path, sm64::join(dir, sm64::relative(main_folder, path)));
    }
}

void write_manifest(string dir) {
    nlohmann::json addon = {{
        "bit", {
            { "name", "Moon64" },
            { "authors", { "UnderVolt", "Nintendo" } },
            { "description", "SM64 Default Addon" },
            { "version", 1.0 },
            { "readOnly", true }
        }}
    };
    sm64::writeFile(sm64::join(dir, "properties.json"), addon.dump(4));
}

void build_addon( char* rom, char* soundbank, char* soundplayer, char* dir ){
    string rom_path(rom);
    string temp_dir(dir);

    sm64::rm(temp_dir);

    cout << "Extracting to " << temp_dir << endl;
    read_assets(rom, temp_dir);

    vector<string> files;
    sm64::dirscan(temp_dir, files);

    for(auto& file : files){
        string ext = sm64::extname(file);
        for(auto& transformer : transformers)
            if(ext == transformer.name)
                transformer.callback(sm64::basename(file), file);
    }

    copy_defaults(temp_dir);
    compile_sound(string(soundplayer), string(soundbank), temp_dir);
    copy_assets(temp_dir);
    remove_unused(temp_dir);
    write_manifest(temp_dir);
    cout << "Done!" << endl;
}

int main(int argc, char *argv[]) {

    if(argc < 5){
        cout << "Usage: " << argv[0] << " <rom> <soundbanks> <sequences> <output> [...defaults]" << endl;
        return 1;
    }

    for(int i = 5; i < argc; i++){
        std::string path = argv[i];
        if(fs::exists(path)){
            defaults.push_back(path);
        }
    }

    build_addon(argv[1], argv[2], argv[3], argv[4]);

    return 0;
}