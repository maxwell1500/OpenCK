import re

path = "C:/Users/max/Projects/OpenCK/openck/libs/files/esm/armorrecord.cpp"
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

lines = content.split('\n')

# Check if the pattern matches
line = lines[122]
closing = "    }"
print(f"line 123: {repr(line)}")
print(f"rstrip: {repr(line.rstrip())}")
print(f"match closing: {line.rstrip() == closing}")

# Look ahead from line 124 (index 123)
j = 123
while j < len(lines) and lines[j].strip() == '':
    print(f"blank {j}: {repr(lines[j])}")
    j += 1
print(f"non-blank line {j}: {repr(lines[j])}")

pat = r"void \w+Record::save"
print(f"regex pat: {pat}")
print(f"match: {bool(re.match(pat, lines[j].strip()))}")
