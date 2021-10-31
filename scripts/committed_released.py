"""Use launchpadlib to change bugs that have a "Fix Committed" status to
"Fix Released". Authorize as "Change non-private data" when Launchpad asks for
it.

Requirements:
    - launchpadlib
        <https://help.launchpad.net/API/launchpadlib#Installation>
        pip install launchpadlib

Might help on odd installs:
    pip install --upgrade pip
    pip install keyrings.alt
"""

from optparse import OptionParser

parser = OptionParser(usage='%prog version [bugs_to_exclude]')
options, args = parser.parse_args()
if len(args) < 1:
    parser.error('No version defined')
message = 'Fixed in DC++ ' + args[0] + '.'
exclude = list()
if len(args) > 1:
    exclude = [int(string) for string in args[1:]]

from launchpadlib.launchpad import Launchpad
from launchpadlib.errors import HTTPError

launchpad = Launchpad.login_with('DC++ release script', 'production')
project = launchpad.projects['dcplusplus']

bug_tasks = project.searchTasks(status='Fix Committed')

changed = 0
unchanged = list()
total = 0

for bug_task in bug_tasks:
    if bug_task.bug.id in exclude:
        continue
    total = total + 1
    try:
        bug_task.status = 'Fix Released'
        bug_task.bug.newMessage(content=message)
        bug_task.lp_save()
        changed = changed + 1
    except HTTPError:
        unchanged.append(bug_task.bug.id)

print '%d/%d bugs have been changed.' % (changed, total)
for identifier in unchanged:
    print 'Bug #%d could not be changed.' % identifier
