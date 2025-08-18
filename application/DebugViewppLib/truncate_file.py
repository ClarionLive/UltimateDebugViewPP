# Truncate the file to remove dead code
with open(r"C:\dev\DebugViewPP\application\DebugViewppLib\SyntaxHighlighter.cpp", "r", encoding="utf-8") as f:
    lines = f.readlines()

# Keep only up to line 2238 (index 2237)
lines = lines[:2238]

with open(r"C:\dev\DebugViewPP\application\DebugViewppLib\SyntaxHighlighter.cpp", "w", encoding="utf-8") as f:
    f.writelines(lines)

print("File truncated to 2238 lines")