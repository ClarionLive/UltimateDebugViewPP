# Multi-Computer Development Setup for UltimateDebugViewPP

This guide explains how to work on UltimateDebugViewPP across multiple computers (laptop and desktop).

## Initial Setup on Second Computer

### 1. Clone the Repository
```bash
git clone https://github.com/peterparker57/UltimateDebugViewPP.git
cd UltimateDebugViewPP
```

### 2. Set Git Configuration
```bash
git config user.email "peterparker57@users.noreply.github.com"
git config user.name "peterparker57"
```

### 3. Install Build Requirements
- **Visual Studio 2022** (Community Edition or higher)
  - Include "Desktop development with C++" workload
  - Include Windows SDK
- **CMake** (already included in repo at cmake-3.27.7-windows-x86_64)
- **Git for Windows**

### 4. Build the Project
```bash
# Generate Visual Studio solution
cmake-3.27.7-windows-x86_64\bin\cmake.exe -DCMAKE_INSTALL_PREFIX=install -G "Visual Studio 17 2022" -B vs2022 .

# Build Release version
cmake-3.27.7-windows-x86_64\bin\cmake.exe --build vs2022 --config Release --target UltimateDebugviewpp

# The executable will be in: bin\Release\UltimateDebugviewpp.exe
```

## Daily Workflow - Syncing Between Computers

### Before Switching Computers
Always commit and push your changes before moving to another computer:

```bash
# Check what has changed
git status

# Stage all changes
git add -A

# Commit with descriptive message
git commit -m "Description of changes"

# Push to GitHub
git push
```

### After Switching Computers
Pull the latest changes when you start working on a different computer:

```bash
# Pull latest changes
git pull

# If there were CMake or source changes, rebuild
cmake-3.27.7-windows-x86_64\bin\cmake.exe --build vs2022 --config Release --target UltimateDebugviewpp
```

## Quick Commands Reference

### Build Commands
```bash
# Full rebuild
cmake-3.27.7-windows-x86_64\bin\cmake.exe --build vs2022 --config Release --target UltimateDebugviewpp --clean-first

# Build and run
cmake-3.27.7-windows-x86_64\bin\cmake.exe --build vs2022 --config Release --target UltimateDebugviewpp
start bin\Release\UltimateDebugviewpp.exe

# Build test programs
cmake-3.27.7-windows-x86_64\bin\cmake.exe --build vs2022 --config Release --target DebugViewTestMessages
cmake-3.27.7-windows-x86_64\bin\cmake.exe --build vs2022 --config Release --target TestClarionMessages
```

### Git Commands
```bash
# Check current branch and status
git status

# View recent commits
git log --oneline -10

# Create new branch for feature work
git checkout -b feature/new-feature

# Switch back to develop
git checkout develop

# Stash changes temporarily (if needed)
git stash
git stash pop
```

## Important Notes

1. **Always commit before switching**: Uncommitted changes won't be available on the other computer
2. **Pull before starting work**: Ensure you have the latest changes from the other computer
3. **Use descriptive commit messages**: This helps track what was done on each computer
4. **Check build after pulling**: If CMakeLists.txt or source files changed, rebuild the project

## Troubleshooting

### Merge Conflicts
If you accidentally make changes on both computers without syncing:
```bash
# Pull and merge
git pull

# If there are conflicts, resolve them in the files
# Then:
git add -A
git commit -m "Resolved merge conflicts"
git push
```

### Build Issues After Pulling
```bash
# Clean and rebuild
cmake-3.27.7-windows-x86_64\bin\cmake.exe --build vs2022 --config Release --target UltimateDebugviewpp --clean-first

# Or regenerate the entire solution
rm -rf vs2022
cmake-3.27.7-windows-x86_64\bin\cmake.exe -DCMAKE_INSTALL_PREFIX=install -G "Visual Studio 17 2022" -B vs2022 .
```

### Authentication Issues
If you have trouble pushing/pulling, ensure you're using a GitHub Personal Access Token or SSH keys for authentication.

## Project Structure
- `bin/Release/` - Built executables
- `application/` - Source code
- `Distribution/` - Release files
- `vs2022/` - Generated Visual Studio solution (not in git)
- `cmake-3.27.7-windows-x86_64/` - CMake tools (included in repo)