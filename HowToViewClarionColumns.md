# How to View Clarion Columns in DebugView++

## Method 1: Right-Click on Column Headers
1. Start UltimateDebugviewpp.exe
2. Right-click on any column header in the list view
3. A context menu will appear with all available columns
4. Check the Clarion-specific columns you want to see:
   - **Thread** - Shows the thread ID from Clarion messages
   - **App** - Shows the application name
   - **Module** - Shows the module name (.clw file)
   - **Procedure** - Shows the procedure/function name
   - **Prefix** - Shows the filter prefix (DB, UI, etc.)

## Method 2: View Menu (if available)
The columns may also be accessible from the View menu in the main menu bar.

## Column Visibility
- Standard messages will show empty values in Clarion columns
- Clarion messages (those starting with [||]X,) will populate these columns
- You can show/hide columns at any time
- Column settings are saved and restored when you restart the application

## Testing
1. Run UltimateDebugviewpp.exe
2. Enable the Clarion columns (Thread, App, Module, Procedure, Prefix)
3. Run DebugViewTestMessages.exe
4. You should see Clarion messages with all fields populated

## Tips
- You can reorder columns by dragging the column headers
- Column widths can be adjusted by dragging the column borders
- The Date column is hidden by default but can be enabled if needed
- Consider creating different views for Clarion vs non-Clarion debugging