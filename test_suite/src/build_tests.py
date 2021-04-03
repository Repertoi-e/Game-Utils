import os
import re

def comment_remover(text):
    def replacer(match):
        s = match.group(0)
        if s.startswith('/'):
            return " " # Note: a space and not an empty string
        else:
            return s
            
    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )
    
    return re.sub(pattern, replacer, text)
    
# @Volatile Same function in test.h
def get_short_file_path(path):
    index = path.rfind("src" + os.path.sep)
    if index == -1:
        index = path.rfind(os.path.sep)
        index += 1; # Skip /
    else:
        index += 4; # Skip src/
    
    return path[index:]
    
build_test_table_contents = ""

def output_test(file, test_name):
    global build_test_table_contents

    func_name = "test_" + test_name

    build_test_table_contents += f"    extern void {func_name}();\n"
    build_test_table_contents += f'    array_append(*g_TestTable[string("{file}")], {{"{test_name}", {func_name}}});\n'
    
def handle_file(path, file_name):
    global build_test_table_contents
    
    with open(path, "r", encoding = "utf8") as f :
        contents = f.read()

    name_map = dict()

    # Before removing comments let's check for notes
    for l_num, l in enumerate(contents.split("\n")):
        if ":build_tests:" in l:
            l_stripped = " ".join(l.split())
            _, rest = l_stripped.split(":build_tests: ")
            
            tokens = rest.split()

            name = tokens[0]
            if tokens[1] != "->":
                for _ in range(10):
                    print("###################################################")
                print(f'Invalid note at {file_name}:{l_num}.')
                print(f'Line was: "{l}" (expected an arrow -> after the first name)')
                for _ in range(10):
                    print("###################################################")
            alternative_names = tokens[2:]
            
            name_map[name] = alternative_names

    print("-> Name map: ", name_map)
        
    # Robustly remove comments so we don't catch tests which aren't really defined
    contents = comment_remover(contents)
     
    #
    # We ignore the entire file if it contains :IgnoreTestFile: anywhere (e.g in a comment)
    #
    if ":IgnoreTestFile:" in contents: return
    
    #
    # We ignore a test if preceeded by "x". 
    # Usually commenting out is enough (because we remove comments ^ above)
    # but if we need it for some strange reason it's here.
    #
    for t in re.finditer(r"(?<!x)TEST(\(.*?\))", contents):
        # @Volatile: Identifier must match the macro in test.h
        test_name = t.group(1)[1:-1]
        
        short_file_path = get_short_file_path(path)
        
        if test_name in name_map:
            for alternative_name in name_map[test_name]:
                output_test(short_file_path, alternative_name)
        else:
            output_test(short_file_path, test_name)
        
        print(file_name, test_name)
  

for root, dirs, files in os.walk("./tests"):
    for file in files:
        if file.endswith(".cpp"):
            handle_file(os.path.join(root, file), file)
           
file_content = (''
'#include "test.h"\n'
'\n'
'void build_test_table() {\n'
f'{build_test_table_contents}\n'
'}\n')

with open("build_test_table.cpp", "w") as f:
    f.write(file_content)
