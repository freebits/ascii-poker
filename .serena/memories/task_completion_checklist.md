# Task Completion Checklist

## When a coding task is completed:

### 1. Build and Compile
```bash
make clean && make all
```
- Ensure no compilation errors
- Review and address any warnings (especially buffer-related warnings are acceptable as they're defensive)

### 2. Code Quality Checks
- No unused variables (except in intentional stubs)
- No memory leaks (for complex changes, run valgrind)
- Proper error handling for system calls
- Thread-safe access to shared state (mutex locks)

### 3. Testing
- **Manual testing**: Run server and at least 2 clients
- **Protocol testing**: Verify STATE messages display correctly
- **Game flow**: Test at least one complete hand (preflop through showdown)
- **Edge cases**: Test fold, all-in, raise scenarios

### 4. Debug Output
- If debugging, ensure debug output goes to stderr (not stdout)
- Clean up or comment out verbose debug printf/fprintf before final commit

### 5. Documentation
- Update README.md if functionality changes
- Update QUICKSTART.md if user workflow changes
- Keep COMPARISON.md in sync if comparing versions

### 6. Git Workflow (if applicable)
```bash
git add <files>
git commit -m "Descriptive message"
git push
```

## Before Marking Task Complete
✅ Compiles without errors
✅ Manual testing passed
✅ No obvious memory leaks
✅ Thread safety verified for concurrent access
✅ Documentation updated if needed
✅ Code follows project style conventions
