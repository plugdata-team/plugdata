import os

directory = './vanilla'

for filename in os.listdir(directory):
    f = os.path.join(directory, filename)
    # checking if it is a file
    if os.path.isfile(f) and f.endswith('.md'):
        file = open(f, "r")
        content = file.read()
        content = content.replace("\n.\n", "\n")

        open(f, "w").write(content);
