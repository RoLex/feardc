#@PydevCodeAnalysisIgnore

def gen_changelog(target, source, env):
	import re, cgi, html

	f_template = open(str(source[1]), "r")
	template = f_template.read()
	f_template.close()
	template = template.split("<!-- contents -->", 1)

	start_head = template[0] + """
<style type=\"text/css\">
li { margin-left: auto; margin: 0em 0em 0em 0em; }
</style>

<h1>
	<img src=\"Changelog.ico\" width=\"16\" height=\"16\" alt=\"DC++ Changelog\"/>
	DC++ Changelog
</h1>
See the version history of DC++ below.

"""

	start_change = "  <li>%(change)s"
	bugzilla_text = "  <li><a href=\"http://dcpp.net/bugzilla/show_bug.cgi?id=%(bug_id)s\" target=\"_blank\" class=\"external\">[Bugzilla bug %(bug_id)s]</a> %(change)s"
	launchpad_text = "  <li><a href=\"https://bugs.launchpad.net/dcplusplus/+bug/%(bug_id)s\" target=\"_blank\" class=\"external\">[Launchpad bug %(bug_id)s]</a> %(change)s"
	change = " %(change)s"
	end_change = "</li>\n"

	start_version = "<h2>%(version)s <span style=\"color: gray;\">(%(date)s)</span></h2>\n<ul>\n"
	end_version = "</ul>\n\n"

	start_warning_end = "  <li><span style=\"color: red;\">%(change)s</span></li>\n"

	new_version_pattern = re.compile("^.*?-- (?P<version>.*?) (?P<date>.*?) --")
	new_change = re.compile(r"^\* (?P<change>.*?)$")
	bugzilla_change = re.compile(r"^\* \[B#(?P<bug_id>\d+?)\] (?P<change>.*?)$")
	launchpad_change = re.compile(r"^\* \[L#(?P<bug_id>\d+?)\] (?P<change>.*?)$")
	continue_change = re.compile("^\w*?(?P<change>.*?)$")
	warning_change = re.compile("^(?P<change>[^ ].*?)$")

	fp_txt = open(str(source[0]), 'r')

	fp_html = open(str(target[0]), 'w')
	fp_html.write(start_head)

	open_change_state = False
	close_version = False
	start = False

	for line in fp_txt:
		line = html.escape(line.strip(), False)
		if not line:
			if open_change_state:
				fp_html.write(end_change)
				open_change_state = False
			continue
			
		mObj = new_version_pattern.match(line)
		if mObj and mObj.groupdict()["date"] :
			if close_version:
				if open_change_state:
					fp_html.write(end_change)
				fp_html.write(end_version)
			start = True
			close_version = True
			open_change_state = False
			open_warning_state = False
			fp_html.write(start_version % mObj.groupdict())
			continue
		
		if not start:
			continue
		
		mObj = bugzilla_change.match(line)
		if mObj:
			if open_change_state:
			    fp_html.write(end_change)
			fp_html.write(bugzilla_text % mObj.groupdict())
			open_change_state = True
			continue

		mObj = launchpad_change.match(line)
		if mObj:
			if open_change_state:
			    fp_html.write(end_change)
			fp_html.write(launchpad_text % mObj.groupdict())
			open_change_state = True
			continue

		mObj = new_change.match(line)
		if mObj: # A new change is found: Close Open Warning or Changes.
			if open_change_state:
				fp_html.write(end_change)
			fp_html.write(start_change % mObj.groupdict())
			open_change_state = True
			continue 
		
		mObj = continue_change.match(line)
		if mObj and open_change_state: #A continutaion of an change (multiline change).
			fp_html.write(change % mObj.groupdict())
			continue
			
		mObj = warning_change.match(line)
		if mObj:
			if open_change_state:
				fp_html.write(end_change_state)
			fp_html.write(start_warning_end % mObj.groupdict()) 
			continue

	if open_change_state:
		fp_html.write(end_change)
	if close_version:
		fp_html.write(end_version)
	fp_html.write(template[1])
	fp_html.close()
	fp_txt.close()
