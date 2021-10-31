def get_rev_id(env):
    """Attempt to get information about the repository, via the "hg log"
    command. Its output is formatted via the "-T" parameter (see "hg templates"
    for details).

    :return: Version information string, or "[unknown]" on failure.
    :rtype: str.
    """

    try:
        import subprocess
        ret = subprocess.check_output(
            'hg log -r tip -T "{node | short} - {date | isodate}"',
            shell=True
        )
        if ret:
            return ret
    except:
        pass
    return '[unknown]'


def gen_rev_id(target, source, env):
    f = open(str(target[0]), 'w')
    f.write('#define DCPP_REVISION "%s"\n' % get_rev_id(env))
    f.close()
