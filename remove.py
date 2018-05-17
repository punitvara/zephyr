import re,os,shutil

def finddir():
    for path, dirs, files in os.walk('./tests/posix/mutex'):  
            for names in files:
                if names.endswith('.c'):
                    #for line in open(os.path.join(path, names), "rw"):
                    with open(os.path.join(path, names), 'r') as f:
                        f.seek(0, 0)
                        read_data = f.read()
                        for line in open(os.path.join(path, names), 'r+'):
                            fout = open(os.path.join(path, names)).read()
                            if re.findall("zassert_", line):
                                xyz = re.search(r'\\n\"\)\;', line)
                                if xyz:
                                    replace = re.sub(r'\\n\"\)\;', r'\"\)\;', line)
                                    line = line.restrip(replace)

def main():
    print("Hellow wor")
    finddir()

if __name__ == "__main__":
        main()
