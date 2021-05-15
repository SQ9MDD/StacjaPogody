import datetime
import hashlib

FILENAME_BUILDNO = 'versioning'
FILENAME_VERSION_H = 'include/version.h'
MAIN_FILE = 'src/main.cpp'
SECONDARY_FILE = 'src/taskWebServer.cpp'

tm = datetime.datetime.today()
md5_hash = hashlib.md5()
version = 'v.1.'
sub_version = 0
build_version = 0
build_date = str(tm.year)[-4:]+('0'+str(tm.month))[-2:]+('0'+str(tm.day))[-2:]
build_no = 0
main_checksum_last = ""
main_checksum_now = ""
secondary_checksum_last = ""
secondary_checksum_now = ""

def calc_checksum(file_name):
  a_file = open(file_name,"rb")
  content = a_file.read()
  md5_hash.update(content)
  digest = md5_hash.hexdigest()
  return(digest)

main_checksum_now = calc_checksum(MAIN_FILE)
secondary_checksum_now = calc_checksum(SECONDARY_FILE)
try:
  with open(FILENAME_BUILDNO) as f:
    build_no = int(f.readline())
    main_checksum_last = f.readline().strip()
    secondary_checksum_last = f.readline().strip()
    if(main_checksum_now != main_checksum_last or secondary_checksum_now != secondary_checksum_last):   # check files for changes here :)
      build_no = build_no + 1
except:
    print('Starting build number from 1..')
    build_no = 1

sub_version = int(build_no / 100)
build_version = int((build_no % 100) / 10)
version = version + str(sub_version) + "." + str(build_version)

with open(FILENAME_BUILDNO, 'w+') as f:
    f.write(str(build_no) + "\n" + str(main_checksum_now) + "\n" + secondary_checksum_now + "\n")
    print('Version: {} \nBuild number: {}\n'.format(version,build_no))

hf = """
#ifndef BUILD_NUMBER
  #define BUILD_NUMBER "{}"
#endif
#ifndef VERSION
  #define VERSION "{} - {}"
#endif
#ifndef VERSION_SHORT
  #define VERSION_SHORT "{}"
#endif
""".format(build_no, version, build_date, version)
with open(FILENAME_VERSION_H, 'w+') as f:
    f.write(hf)