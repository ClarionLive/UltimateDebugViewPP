# Clarion Debug Format Specification for DebugView++

## New Format (Simple Key:Value)

Each debug message should be a single, self-contained string with all necessary information.

### Format Structure
```
[||]X,T:<threadId>,P:<procedureName>,M:<moduleName>,A:<appName>,PRE:<prefix>,MSG:<message>
```

### Required Fields
- `[||]X` - Clarion message identifier (must be at the start)
- `T:<threadId>` - Thread ID (numeric)
- `P:<procedureName>` - Name of the procedure
- `M:<moduleName>` - Module filename (e.g., Module.clw)
- `A:<appName>` - Application name (e.g., MyApp.exe)
- `MSG:<message>` - The actual debug message (can contain commas, colons, etc.)

### Optional Fields
- `PRE:<prefix>` - Optional prefix for filtering (e.g., DB, UI, WORK)

### Examples

With prefix:
```
[||]X,T:1234,P:InitDatabase,M:MainModule.clw,A:MyApp.exe,PRE:DB,MSG:Connecting to database...
[||]X,T:5678,P:CreateWindow,M:UIModule.clw,A:MyApp.exe,PRE:UI,MSG:Window created successfully
```

Without prefix:
```
[||]X,T:3456,P:LogEvent,M:LogModule.clw,A:MyApp.exe,MSG:Standard log entry
```

Message with special characters:
```
[||]X,T:1234,P:ParseData,M:DataModule.clw,A:MyApp.exe,PRE:DATA,MSG:Processing: ID=123, Name=John, Value=$456.78
```

## Clarion Template Code Example

```clarion
! Build the debug message in the new format
DebugString  STRING(2000)

! Required fields
DebugString = '[||]X'
DebugString = CLIP(DebugString) & ',T:' & THREAD()
DebugString = CLIP(DebugString) & ',P:' & ProcedureName
DebugString = CLIP(DebugString) & ',M:' & ModuleName
DebugString = CLIP(DebugString) & ',A:' & ApplicationName

! Optional prefix
IF Prefix
    DebugString = CLIP(DebugString) & ',PRE:' & CLIP(Prefix)
END

! The message (always last so it can contain commas)
DebugString = CLIP(DebugString) & ',MSG:' & CLIP(YourDebugMessage)

! Send to DebugView++
OutputDebugString(DebugString)
```

## Benefits of This Format

1. **Self-contained**: Each message has all information needed
2. **Simple parsing**: Key:value pairs are easy to parse
3. **No state tracking**: No need to track procedures/modules across messages
4. **Filterable**: Can easily filter by prefix, procedure, module, etc.
5. **Extensible**: Easy to add new fields in the future

## DebugView++ Display

When DebugView++ receives a Clarion-formatted message, it will:

1. Parse the message and extract all fields
2. Display the fields in appropriate columns:
   - Thread column: Shows the thread ID
   - Procedure column: Shows the procedure name
   - Module column: Shows the module name
   - App column: Shows the application name
   - Prefix column: Shows the prefix (if present)
   - Message column: Shows the actual debug message

3. Regular (non-Clarion) messages will leave these columns empty

## Column Configuration

Users can:
- Show/hide individual columns via View menu
- Save column configurations as profiles
- Switch between "Standard" and "Clarion" column layouts
- Resize and reorder columns as needed

## Testing

To test your Clarion template changes:

1. Update your template to use the new format
2. Run your Clarion application
3. Start UltimateDebugviewpp.exe
4. Enable the Clarion-specific columns (View menu)
5. Your debug messages should appear with all fields properly parsed

## Notes

- The MSG field should always be last because it might contain commas
- Thread IDs can be decimal or hexadecimal (0x prefix)
- All text after MSG: is treated as the message, preserving any special characters
- Empty prefix is allowed (just omit the PRE field)