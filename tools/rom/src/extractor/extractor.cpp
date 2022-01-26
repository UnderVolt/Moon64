#include "extractor.h"

#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include "json.hpp"
#include "utils.h"
#include "sound.h"
#include "assets_table.h"

extern "C" {
#include "n64/libmio0.h"
#include "n64/n64graphics.h"
#include "n64/skyconv.h"
}

using namespace std;
using json = nlohmann::json;

enum DataType {
    RAW = 2,
    TEXTURE = 4
};

struct AssetEntry {
    string path;
    long addr;
    int size;
    vector<int> meta;
};

map<string, vector<AssetEntry>> assets;
char* rom_data = NULL;

void read_rom(json jobj, const char* rom_file){
    ifstream rom(rom_file, std::ios::binary);
    vector<char> bytes((istreambuf_iterator<char>(rom)), (istreambuf_iterator<char>()));
    rom.close();
    rom_data = (char*) malloc(bytes.size());
    memcpy(rom_data, bytes.data(), bytes.size());

    for(auto asset : jobj.items()){
        string key = asset.key();
        auto value = asset.value();

        DataType type = (DataType) value.size();
        json mio = value[type - 1];

        if(!mio.contains("us")) continue;

        auto addr = mio["us"];
        bool isMIO = addr.size() > 1;

        AssetEntry entry = {
            .path = key,
            .addr = addr[int(isMIO)],
            .size = type == TEXTURE ? value[2] : value[0]
        };

        if(type == TEXTURE)
            entry.meta = { value[0], value[1] };

        assets[ isMIO ? PARSE(addr[0]) : "None" ].push_back(entry);
    }

}

void read_assets(const char* rom_file, string basedir){
    bind_engine();
    json jobj = json::parse(string(AssetsTable.file_data));
    read_rom(jobj, rom_file);

    for( auto &entry : assets ){
        string mio = entry.first;
        char* input = NULL;

        if(mio == "@sound") {
            input = rom_data;
            auto ctl = jobj["@sound ctl us"];
            auto tbl = jobj["@sound tbl us"];
            MoonUtils::write(MoonUtils::join(basedir, "sound/sound_data.ctl"), rom_data + (int)ctl[1]["us"][0], (int)ctl[0]);
            MoonUtils::write(MoonUtils::join(basedir, "sound/sound_data.tbl"), rom_data + (int)tbl[1]["us"][0], (int)tbl[0]);
            vector<string> samples;
            for(auto& asset : entry.second){
                samples.push_back(MoonUtils::format("%s:%d", MoonUtils::join(basedir, asset.path).c_str(), asset.addr));
            }
            disassemble_sound(basedir, samples);
            continue;
        }

        if(mio != "None") {
            mioO_data* data = new mioO_data;
            mio0_decode_file(rom_file, stoi(mio), data);
            if(data->data != NULL){
                input = (char*) malloc(data->size);
                memcpy(input, data->data, data->size);
            }
        } else {
            input = rom_data;
        }

        for(auto& asset : entry.second){
            string path = asset.path;
            long addr = asset.addr;
            int size = asset.size;

            if(path[0] == '@') continue;

            string out = MoonUtils::join(basedir, path);
            MoonUtils::mkdir(MoonUtils::dirname(out));
            MoonUtils::write(out, input + addr, size);

            cout << "Extracted " << path << " to " << out << endl;

            if(path.find(".png") != string::npos){
                vector<string> fmt = MoonUtils::split(path, '.');
                bool is_skybox = CONTAINS(path, "skyboxes");
                bool is_cake   = CONTAINS(path, "cake");
                if(is_skybox || is_cake){
                    string imagetype = is_skybox ? "sky" : "cake";
                    string outpath   = MoonUtils::join(MoonUtils::dirname(out), MoonUtils::basename(out)) + ".rgba16.png";
                    skyconv(CNV(out), CNV(outpath), int(!is_skybox));
                } else {
                    char* args[] = {
                        NULL,
                        "-e",
                        CNV(out),
                        "-g",
                        CNV(out),
                        "-f",
                        CNV(fmt[fmt.size() - 2]),
                        "-w",
                        CNV(to_string(asset.meta[0])),
                        "-h",
                        CNV(to_string(asset.meta[1]))
                    };
                    DISPATCH(n64graphics, args);
                }
            }
        }
    }
}