# fsqlf Attribution and License Compliance

## Overview
DebugView++ uses the Free SQL Formatter (fsqlf) library for SQL formatting functionality. fsqlf is licensed under the GNU Lesser General Public License version 3 (LGPL v3).

## Library Information
- **Name**: Free SQL Formatter (fsqlf)
- **Author**: dnsmkl
- **Source**: https://github.com/dnsmkl/fsqlf
- **License**: GNU LGPL v3
- **Usage**: Dynamically linked library (fsqlf.dll)

## Required Actions for LGPL Compliance

### 1. Update About Dialog
Add the following attribution to the DebugView++ About dialog:

```
This software uses:
• Free SQL Formatter (fsqlf) by dnsmkl
  https://github.com/dnsmkl/fsqlf
  Licensed under GNU LGPL v3
  Used for SQL syntax formatting in the detail pane
```

### 2. Include License Files
The following files must be included in the DebugView++ distribution:

- [ ] `licenses/fsqlf-LICENSE.txt` - Copy of the LGPL v3 license from fsqlf
- [ ] `licenses/THIRD_PARTY_LICENSES.txt` - Consolidated list of all third-party licenses

### 3. Update README/Documentation
Add a section about third-party components:

```markdown
## Third-Party Components

DebugView++ includes the following third-party libraries:

### SQL Formatting
SQL syntax formatting is provided by Free SQL Formatter (fsqlf)
- License: GNU LGPL v3
- Source: https://github.com/dnsmkl/fsqlf
- The fsqlf.dll library provides automatic SQL statement formatting with proper indentation and keyword alignment
```

### 4. Distribution Requirements
When distributing DebugView++:

- [ ] Include `fsqlf.dll` in the release package
- [ ] Include the LGPL v3 license text
- [ ] Ensure fsqlf.dll remains a separate file (not embedded)
- [ ] Do not statically link fsqlf (keep it as a DLL)

## Implementation Details

### Current Integration
- fsqlf.dll is loaded dynamically at runtime
- Located in the same directory as DebugView++ executable
- Falls back to simple formatter if DLL is not available
- No static linking - maintains license separation

### Files Modified for Integration
- `application/DebugViewppLib/SqlFormatter.cpp` - Dynamic DLL loading
- `application/DebugViewppLib/SyntaxHighlighter.cpp` - Calls SQL formatter
- `fsqlf_dll/` - Build directory for fsqlf DLL

## Benefits of LGPL Dynamic Linking
- DebugView++ source code remains under its original license
- Users can update/replace fsqlf.dll independently
- No requirement to open-source DebugView++ code
- Can be used in commercial applications

## Reusing fsqlf.dll in Other Projects
The fsqlf.dll can be used in other projects with the same requirements:
1. Include attribution in about/credits
2. Distribute the LGPL license
3. Keep as dynamic library (don't statically link)
4. Provide link to fsqlf source code

## TODO Checklist
- [ ] Add attribution to About dialog
- [ ] Create `licenses/` directory
- [ ] Copy LGPL v3 license to `licenses/fsqlf-LICENSE.txt`
- [ ] Update main README.md with third-party components section
- [ ] Add fsqlf.dll to installer/distribution package
- [ ] Test that application works when fsqlf.dll is missing (fallback)
- [ ] Consider adding "SQL Formatting provided by fsqlf" tooltip in UI

## Notes
- The LGPL v3 is specifically designed to allow proprietary software to use open-source libraries via dynamic linking
- Since fsqlf is a separate DLL, there's no "viral" effect on the DebugView++ license
- Users benefit from professional SQL formatting without licensing complications
- The integration includes proper error handling if the DLL is missing or fails to load