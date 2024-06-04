import subprocess

revision = (
    subprocess.check_output(["git", "rev-parse", "HEAD"])
    .strip()
    .decode("utf-8")[:7]
)

def modify_system_task_h():
    result = ""
    with open("./src/system_task.h", 'r') as file:
        for line in file:
            if "String firmware_ =" in line:
                result += f"    String firmware_ = \"{revision}\";\n"
            else:
                result += line

    with open("./src/system_task.h", 'w') as output:
        output.write(result)


modify_system_task_h()