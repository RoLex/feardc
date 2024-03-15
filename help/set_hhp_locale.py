def set_hhp_locale(target, lcid):
    print("Setting " + target + " to the 0x%04X locale" % lcid)
    f = open(target, "r+")
    contents = f.read()
    f.seek(0)
    f.write(contents.replace("Language=0x0409", "Language=0x%04X" % lcid))
    f.close()
