#include "sound.h"

#include <vector>
#include <string>
#include "disassemble_sound.h"
#include "assemble_sound.h"
#include "assets_table.h"
#include "sequences_table.h"
#include <python3.10/Python.h>
#include "utils.h"

extern "C" {
#include "n64/aifc_decode.h"
}

using namespace std;

#define BIND_FUNC(name, desc, cb) { name, (PyCFunction) cb, METH_VARARGS | METH_KEYWORDS, desc }

static PyObject *bind_aifc_decode(PyObject *self, PyObject *args, PyObject *kwargs){
    PyObject *new_status;
    char *path;
    char *output;
    static char *kwlist[] = {
        "path",
        "output",
        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s", kwlist, &path, &output)) return NULL;

    aifc_decode(path, output);
    return Py_None;
}

static PyObject *bridge_print(PyObject *self, PyObject *args, PyObject *kwargs){
    PyObject *new_status;
    char *msg;
    static char *kwlist[] = {
        "msg",
        NULL
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s", kwlist, &msg)) return NULL;

    cout << "Python: " << msg << endl;
    return Py_None;
}


static PyObject *bind_seq_json(PyObject *self, PyObject *args, PyObject *kwargs){
    PyObject *result = NULL;
    result = Py_BuildValue("s", MoonUtils::sys_strdup(SequencesTable.file_data));
    return result;
}

static PyMethodDef methods[] = {
    BIND_FUNC("aifc_decode", "Decode a AIFC file", bind_aifc_decode),
    BIND_FUNC("seq_json",    "Bind json sequences", bind_seq_json),
    BIND_FUNC("print",       "Bridge print", bridge_print),
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef MextDefinition = { PyModuleDef_HEAD_INIT, "mext", "Layer of comunication between C and Python", -1, methods };

PyMODINIT_FUNC PyInit_mext(void) {
    return PyModule_Create(&MextDefinition);
}

void bind_engine(){
    if (PyImport_AppendInittab("mext", PyInit_mext) == -1) {
        fprintf(stderr, "Error: could not extend in-built modules table\n");
        exit(1);
    }

    Py_Initialize();

    PyObject *pmodule = PyImport_ImportModule("mext");
    if (!pmodule) {
        PyErr_Print();
        fprintf(stderr, "Error: could not import module 'mext'\n");
    }
}

void end(){}

void disassemble_sound(string dir, vector<string> samples){
    vector<char*> args = {
        "disassemble_sound.py",
        CNV(MoonUtils::join(dir, "sound/sound_data.ctl")),
        CNV(MoonUtils::join(dir, "sound/sound_data.tbl")),
        "--only-samples"
    };

    for(auto sample : samples){
        args.push_back(CNV(sample));
    }

    wchar_t** wargv = new wchar_t*[args.size()];
    for(int i = 0; i < args.size(); i++)
        wargv[i] = Py_DecodeLocale(args[i], nullptr);
    PySys_SetArgv(args.size(), wargv);
    PyRun_SimpleString(string(DisassembleSound.file_data).c_str());
}

void assemble_banks(string banks, string dir){
    char* cflags = "-DVERSION_US -D_LANGUAGE_C -DNON_MATCHING -DAVOID_UB";

    vector<char*> args = {
        "assemble_banks",
        CNV(MoonUtils::join(dir, "sound/samples/")),
        CNV(banks),
        CNV(MoonUtils::join(dir, "sound/sound_data.ctl")),
        CNV(MoonUtils::join(dir, "sound/sound_data.tbl")),
        cflags,
        "--endian",
        CONST(MoonUtils::endian),
        "--bitwidth",
        CONST(MoonUtils::bitwidth)
    };

    cout << "Assembling sound banks..." << endl;
    wchar_t** wargv = new wchar_t*[args.size()];
    for(int i = 0; i < args.size(); i++)
        wargv[i] = Py_DecodeLocale(args[i], nullptr);
    PySys_SetArgv(args.size(), wargv);
    PyRun_SimpleString(string(AssembleSound.file_data).c_str());
}

void assemble_m64(string banks, string dir){
    char* cflags = "-DVERSION_US -D_LANGUAGE_C -DNON_MATCHING -DAVOID_UB";

    vector<char*> args = {
        "assemble_m64",
        "--sequences",
        CNV(MoonUtils::join(dir, "sound/sequences.bin")),
        CNV(MoonUtils::join(dir, "sound/bank_sets")),
        CNV(banks),
        "owo.json",
    };

    std::vector<std::string> dirfiles;
    MoonUtils::dirscan(dir, dirfiles);
    for( auto &file : dirfiles )
        if( MoonUtils::extname(file) == "m64" ) {
            cout << "Assembling " << file << "..." << endl;
            args.push_back(CNV(file));
        }

    args.push_back(cflags);
    args.push_back("--endian");
    args.push_back(CONST(MoonUtils::endian));
    args.push_back("--bitwidth");
    args.push_back(CONST(MoonUtils::bitwidth));

    wchar_t** wargv = new wchar_t*[args.size()];
    for(int i = 0; i < args.size(); i++)
        wargv[i] = Py_DecodeLocale(args[i], nullptr);
    PySys_SetArgv(args.size(), wargv);
    cout << "Assembling m64..." << endl;
    PyRun_SimpleString(string(AssembleSound.file_data).c_str());
}