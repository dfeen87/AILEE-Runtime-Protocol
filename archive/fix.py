with open('src/l6/FederationExport.cpp', 'r') as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    new_lines.append(line)
    if 'return oss.str();' in line and not found_hex:
        pass # wait for the brace

found_brace = False
for i, line in enumerate(lines):
    if 'return oss.str();' in line:
        pass
