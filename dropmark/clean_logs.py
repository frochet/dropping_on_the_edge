import sys, os
def clean_log(dirpath, cleaning_above_hour, cleaning_above_minute):
    all_remaining_lines = ""
    with open(os.path.join(dirpath, "stdout-tor-1000.log"), 'r') as f:
        for line in f:
            date_text = line[0:18]
            hour = int(date_text.split(' ')[2].split(':')[0])
            minutes = int(date_text.split(' ')[2].split(':')[1])
            if hour >= 0 and minutes >= 20 and hour <= cleaning_above_hour and minutes <= cleaning_above_minute: #get the minutes
                all_remaining_lines += line
    with open(os.path.join(dirpath, "stdout-tor-1000.log"), 'w') as f:
        f.write(all_remaining_lines)


if __name__ ==  "__main__":
    """
    Clean the log removing all entry before 1200 secondsa - webclients do not start
    operating before 1200 virtual seconds

    Clean also the log close to the terminaison time: Signals might be send, then the program
    might be terminated while the signals do not reach the clients
    """
    #pathname is the directory of the shadow simulation
    pathname = sys.argv[1]
    cleaning_above_hour = int(sys.argv[2])
    cleaning_above_minute = int(sys.argv[3])
    in_dir = ""
    print "Cleaning logs below 20 minutes (no streams should be seen) and above {0} hour {1} minute".format(cleaning_above_hour, cleaning_above_minute)
    for dirpath, dirnames, fnames in os.walk(os.path.join(pathname, 'shadow.data/hosts/')):
        for dirname in dirnames:
            if 'relayguard' in dirname or 'relayexit' in dirname:
                in_dir = os.path.join(dirpath, dirname)
                print 'Cleaning log from {0}'.format(dirname)
                clean_log(in_dir, cleaning_above_hour, cleaning_above_minute)
    print "All done. yay!"

