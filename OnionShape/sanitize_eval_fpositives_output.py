import sys, os

"""

Remove duplicate positive relay if they are spaced less than the attack window time

"""

out_dir = "results/"

def sanitize_file(filename, attack_win):
    lines = {} #"'fp':[lines]"
    with open(os.path.join(out_dir, filename), 'r') as f:
        for line in f:
            line_tab = line.split(' ')
            if line_tab[1] in lines:
                previous_line = lines[line_tab[1]][-1]
                if int(line_tab[6])-int(previous_line.split(' ')[6]) >= (attack_win+1)*4*60*60:
                    lines[line_tab[1]].append(line)
            else:
                lines[line_tab[1]] = [line]
    with open(os.path.join(out_dir, "{0}_unordered".format(filename)), 'w') as f:
        for line_list in lines.values():
            for line in line_list:
               f.write(line)
def main_sanitazer():
    pathnames = []
    for dirpath, dirnames, fnames in os.walk(out_dir):
        for fname in fnames:
            if "att_win" in fname: #file always start with attack_win
                pathnames.append(os.path.join(dirpath, fname))
    for pathname in pathnames:
        filename = os.path.basename(pathname)
        if filename[0] == '.' or 'unordered' in filename or (not filename.startswith("att_win") and not filename.startswith("attack")):
            continue
        if filename.startswith("attack"):
            attack_win = int(filename.split('.')[5].split('-')[0])
        else:
            attack_win = int(filename.split('.')[1].split('-')[0])
        sanitize_file(filename, attack_win)


if __name__ == "__main__":
    main_sanitazer()
