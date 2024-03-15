"""This script browses through the index file to generate tables of contents.
"""


def gen_toc(target, source, lcid):
    import codecs
    from html.parser import HTMLParser
    from html.entities import entitydefs
    import re
    from build_util import get_win_cp

    spaces = re.compile("\s+")

    # define our HTML parsing class derived from HTMLParser
    class Parser(HTMLParser):
        text = ""
        link = ""
        in_ul = 0
        keep_data = 0

        def handle_starttag(self, tag, attrs):
            if tag == "a" and self.in_ul:
                # attrs is a list of tuples; each tuple being an
                # (attr-name, attr-content) pair
                for attr in attrs:
                    if attr[0] == "href":
                        self.link = attr[1]
                        self.keep_data = 1
            elif tag == "h3":
                # title of a new section
                self.keep_data = 1
            elif tag == "ul":
                f_hhc.write("\r\n<ul>")
                if len(target) >= 2:
                    f_inc.write("\n<ul>")
                self.in_ul += 1

        def handle_data(self, data):
            if self.keep_data:
                self.text += data

        def handle_entityref(self, name):
            if self.keep_data:
                self.text += entitydefs[name]

        def handle_endtag(self, tag):
            if tag == "a" and self.in_ul:
                # reached the end of the current link entry
                self.text = spaces.sub(" ", self.text).strip()
                f_hhc.write(
                    '\r\n<li> <object type="text/sitemap">'
                    '\r\n<param name="Name" value="'
                )
                f_hhc.write(self.text)
                f_hhc.write('">\r\n<param name="Local" value="')
                f_hhc.write(self.link)
                f_hhc.write('">\r\n</object>')
                if len(target) >= 2:
                    f_inc.write('\n<li><a href="')
                    f_inc.write(self.link)
                    f_inc.write('">')
                    f_inc.write(self.text)
                    f_inc.write("</a></li>")
                self.text = ""
                self.link = ""
                self.keep_data = 0
            elif tag == "h3":
                # title of a new section
                f_hhc.write(
                    '\r\n<li> <object type="text/sitemap">'
                    '\r\n<param name="Name" value="'
                )
                f_hhc.write(self.text)
                f_hhc.write('">\r\n</object>')
                if len(target) >= 2:
                    f_inc.write("\n<li><b>")
                    f_inc.write(self.text)
                    f_inc.write("</b></li>")
                self.text = ""
                self.keep_data = 0
            elif tag == "ul":
                f_hhc.write("\r\n</ul>")
                if len(target) >= 2:
                    f_inc.write("\n</ul>")
                self.in_ul -= 1

    # create file pointers for toc.hhc (target[0]) and - optionally - toc.inc
    # (target[1])
    f_hhc = codecs.open(target[0], "w", get_win_cp(lcid), "replace")
    f_hhc.write(
        """<html>
<head>
</head>
<body>
<object type="text/site properties">
<param name="ImageType" value="Folder">
</object>
"""
    )

    if len(target) >= 2:
        f_inc = codecs.open(target[1], "w", "utf_8")

    parser = Parser()
    f_source = codecs.open(source, "r", "utf_8")
    parser.feed(f_source.read())
    f_source.close()
    parser.close()

    f_hhc.write("\r\n</body>\r\n</html>")
    f_hhc.close()

    if len(target) >= 2:
        f_inc.close()
