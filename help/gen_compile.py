def gen_compile(target, source, env):
    env.Execute(
        '"' + env['asciidoc'] + '" -s -o"' + str(target[0]) +
        '" "' + str(source[0]) + '"'
    )

    f = open(str(source[1]), "r")
    template = f.read()
    f.close()
    template = template.split("<!-- contents -->", 1)

    f = open(str(target[0]), "r")
    contents = f.read()
    f.close()

    import re
    contents = re.sub(
        re.compile(r'<a ([^>]+)>([^<]+)</a>', re.I),
        r'<a \1 target="_blank" class="external">\2</a>',
        contents
    )

    f = open(str(target[0]), "w")
    f.write(template[0])  # header
    f.write("<h1>Compiling DC++</h1>")
    f.write(contents)
    f.write(template[1])  # footer
    f.close()
