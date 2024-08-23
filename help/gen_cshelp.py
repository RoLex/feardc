"""This script browses through every HTML files passed in 'source', looking for
tags that have the "cshelp" argument; target files are then generated for use
by the DC++ help system.
"""


def gen_cshelp(target, source):
    import codecs
    from html.parser import HTMLParser
    from html.entities import entitydefs
    import re
    from build_util import html_to_rtf

    spaces = re.compile(r"\s+")

    # will hold [id, text] pairs
    output = []

    # https://code.djangoproject.com/ticket/23763
    class HTMLParseError(Exception):
        pass

    # define our HTML parsing class derived from HTMLParser
    class Parser(HTMLParser):
        text = ""
        current_tag = ""

        # to handle sub-tags with the same name as the current tag; eg
        # <x cshelp="y">bla <x>bla</x> bla</x>
        count = 0

        def handle_starttag(self, tag, attrs):
            if self.count > 0:
                if tag == "a":
                    # enclose links with quotes so it looks fancier
                    self.text += '"'

                elif tag == "b" or tag == "i" or tag == "u":
                    # the RTF converter understands these tags, so keep them
                    self.text += "<" + tag + ">"
                elif tag == "br" or tag == "p" or tag == "div":
                    self.text += "<br/>"

                if tag == self.current_tag:
                    self.count += 1
                return

            # attrs is a list of tuples; each tuple being an
            # (attr-name, attr-content) pair
            for attr in attrs:
                if attr[0] == "cshelp":
                    output.append([attr[1]])
                    self.current_tag = tag
                    self.count += 1

        def handle_data(self, data):
            if self.count > 0:
                self.text += data

        def handle_entityref(self, name):
            if self.count > 0:
                self.text += entitydefs[name]

        def handle_endtag(self, tag):
            if self.count > 0:
                if tag == "a":
                    # enclose links with quotes so it looks fancier
                    self.text += '"'

                elif tag == "b" or tag == "i" or tag == "u":
                    # the RTF converter understands these tags, so keep them
                    self.text += "</" + tag + ">"
                elif tag == "br" or tag == "p" or tag == "div":
                    self.text += "<br/>"

                if tag == self.current_tag:
                    self.count -= 1
                    if self.count == 0:
                        # reached the end of the current tag
                        output[-1].append(
                            html_to_rtf(spaces.sub(" ", self.text).strip())
                        )
                        self.text = ""
                        self.current_tag = ""

    # parse all source files
    for node in source:
        path = str(node)
        parser = Parser()
        f = codecs.open(path, "r", "utf_8")
        try:
            parser.feed(f.read())
        except HTMLParseError:
            print("gen_cshelp: parse error in " + path)
            raise
        f.close()
        parser.close()

    output.sort()

    # generate cshelp.h (target[0]) and - optionally - cshelp.rtf (target[1])
    f_h = open(str(target[0]), "w")
    f_h.write(
        """// this file contains help ids for field-level help tooltips

#ifndef DCPLUSPLUS_HELP_CSHELP_H
#define DCPLUSPLUS_HELP_CSHELP_H

"""
    )
    number = 11000
    if len(target) >= 2:
        f_rtf = codecs.open(str(target[1]), "w", "utf_8")
    for entry in output:
        f_h.write("#define " + entry[0] + " " + str(number) + "\r\n")
        number += 1
        if len(target) >= 2:
            f_rtf.write(entry[1] + "\r\n")
    f_h.write(
        """
#endif
"""
    )
    f_h.close()
    if len(target) >= 2:
        f_rtf.close()
