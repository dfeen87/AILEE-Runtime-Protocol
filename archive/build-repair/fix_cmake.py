with open("CMakeLists.txt", "r") as f:
    lines = f.readlines()

new_lines = []
in_headers = False
in_sources = False

for line in lines:
    if "src/governor/GovernorEngine.cpp" in line or "src/governor/PolicyRules.cpp" in line or "src/governor/TransferValidator.cpp" in line or "src/rpc/RpcSandboxSimulator.cpp" in line:
        continue
    new_lines.append(line)

with open("CMakeLists.txt", "w") as f:
    f.writelines(new_lines)
