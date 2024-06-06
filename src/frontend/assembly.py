import base64

def base_64(file_path):
    with open(file_path, 'rb') as img_file:
        b64_string = base64.b64encode(img_file.read()).decode("utf-8")
        return "data:image/png;base64," + b64_string


def process_html():
    current_directory = "./src/frontend"
    assembled_html = "const char index_html[] = R\"rawliteral("
    with open("./src/frontend/index.html", 'r') as html_file:
        for html_line in html_file:
            if "index.css" in html_line:
                file_path = current_directory + html_line[html_line.index("href=")+7:-3]
                assembled_html += "\t<style>\n"
                with open(file_path, 'r') as css_file:
                    for line in css_file:
                        if "%" in line:
                            line = line.replace("%", "%%")
                        elif "background-image: url" in line and "https://" not in line:
                            file_path = current_directory + line[27:-3]
                            line = line[:26] + f"'{base_64(file_path)}');"
                        assembled_html += line
                assembled_html += "\t</style>\n"
            elif "index.js" in html_line:
                file_path = current_directory + html_line[18:27]
                assembled_html += "\t<script>\n"
                with open(file_path, 'r') as js_file:
                    for line in js_file:
                        assembled_html += line
                assembled_html += "\t</script>\n"
            elif ".png" in html_line:
                file_path = current_directory + html_line[html_line.index("href=")+7:-5]
                href_tag = html_line[:html_line.index("href=")+6] + base_64(file_path) + "\" />\n"
                assembled_html += href_tag
            else:
                if "%;" in html_line:
                    html_line = html_line.replace("%", "%%")
                assembled_html += html_line

    assembled_html += ")rawliteral\";"

    with open("./src/index.h", 'w') as output:
        output.write(assembled_html)

process_html()