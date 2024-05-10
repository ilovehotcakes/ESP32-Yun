def process_html():
    assembled_html = "const char index_html[] = R\"rawliteral("
    with open("./src/frontend/index.html", 'r') as html_file:
        for html_line in html_file:
            if html_line == "        /* index.css */\n":
                with open("./src/frontend/index.css", 'r') as css_file:
                    for line in css_file:
                        assembled_html += line
            elif html_line == "        /* index.js */\n":
                with open("./src/frontend/index.js", 'r') as js_file:
                    for line in js_file:
                        assembled_html += line
            else:
                assembled_html += html_line

    assembled_html += ")rawliteral\";"

    with open("./src/index.h", 'w') as output:
        output.write(assembled_html)

process_html()