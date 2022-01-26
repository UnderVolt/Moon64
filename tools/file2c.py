from pathlib import Path
import sys
import os
import os.path
import json
import glob

output = [
    "#ifndef APY_H\n\n",
    "static const struct {\n",
    "    unsigned int 	 size;\n",
    "    unsigned char	 file_data[<fs>];\n",
    "} AssembleSound = {\n",
    "    <fs>,\n"
]

lines = open("/home/alex/disks/uwu/Projects/uv/Moon64-Refactor/assets/sound/sequences.json", 'r').readlines()
size = 0
for l in lines:
    output.append('    "{}\\n"\n'.format(l.replace("\"", "\\\"").rstrip('\n')))
    size += len(l) + 1
output.append('    "\0"\n')
output.append("\n};\n")
output.append("\n#endif")

for i in range(len(output)):
    output[i] = output[i].replace("<fs>", f"{size} + 1")

with open(os.path.join("test.h"), 'w') as f:
        f.writelines(output)