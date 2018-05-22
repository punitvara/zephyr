import re,os,shutil

def finddir():
    for path, dirs, files in os.walk('./tests/posix/pthread'):  
            for names in files:
                if names.endswith('.c'):
                    #for line in open(os.path.join(path, names), "rw"):
                    with open(os.path.join(path, names), 'r') as f:
                        lines = f.readlines()
                        f.close()
                        f = open(os.path.join(path, names), 'w')
                        for line in lines:
                            if re.findall("zassert_", line):
                                    xyz = re.search(r'\\n\"\)\;', line)
                                    if xyz:
                                        replace = re.sub(r'\\n\"\)\;', r'");', line)
                                        f.write(replace)
                            else:
                                f.write(line)
                        f.close()

def main():
    print("Hellow wor")
    finddir()

if __name__ == "__main__":
        main()
