import subprocess

revision = (
    subprocess.check_output(["git", "rev-parse", "HEAD"])
    .strip()
    .decode("utf-8")[:7]
)
print("'-D GIT_REV=\"%s\"'" % revision)