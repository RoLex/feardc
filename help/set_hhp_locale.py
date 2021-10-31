def set_hhp_locale(target, lcid):
    print ('Setting ' + target + ' to the 0x%X locale' % lcid)
    f = open(target, 'r+')
    contents = f.read()
    f.seek(0)
    f.write(contents.replace('Language=0x409', 'Language=0x%X' % lcid))
    f.close()
