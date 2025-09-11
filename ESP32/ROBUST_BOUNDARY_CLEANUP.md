# Robust Boundary Cleanup Algorithm

## New Approach - Reverse Search with memcmp

### Problem Analysis
Boundary string: `------WebKitFormBoundaryA7wzDBy3Pl7Um1Nk--`

### Algorithm Steps

#### 1. Reverse Search (Primary)
```c
// Search backwards from end of file
for (long pos = raw_file_size - 10; pos >= 50; pos--) {
    
    // Look for "------" pattern
    if (pos >= 6 && memcmp(&file_content[pos - 6], "------", 6) == 0) {
        
        // Check if followed by "WebKitFormBoundary"
        if (pos + 18 < raw_file_size && 
            memcmp(&file_content[pos], "WebKitFormBoundary", 18) == 0) {
            
            // Found boundary! Find line start
            long line_start = pos - 6;
            while (line_start > 0) {
                char c = file_content[line_start - 1];
                if (c == '\n' || c == '\r') {
                    break;
                }
                line_start--;
            }
            
            clean_file_size = line_start;
            break;
        }
    }
}
```

#### 2. Forward Search (Fallback)
```c
char *boundary_pos = strstr(file_content, "------WebKitFormBoundary");
if (boundary_pos != NULL) {
    // Find line start and cut there
}
```

#### 3. Final Cleanup
```c
// Remove trailing whitespace
while (clean_file_size > 0) {
    char c = file_content[clean_file_size - 1];
    if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
        clean_file_size--;
    } else {
        break;
    }
}
```

## Expected Debug Output

### Successful Cleanup:
```
I (12350) http_server: Raw upload completed. Now cleaning up multipart boundaries...
I (12351) http_server: Searching for multipart boundary in 4567 bytes...
I (12352) http_server: Found boundary pattern at position 4096
I (12353) http_server: Boundary line starts at position 4094
I (12354) http_server: Clean file size after boundary removal: 4094 bytes (was 4567)
I (12355) http_server: Final clean file size after whitespace removal: 4094 bytes
I (12356) http_server: Last 50 bytes of cleaned file:
[Binary data - no boundary text]
```

### If Reverse Search Fails:
```
I (12351) http_server: Searching for multipart boundary in 4567 bytes...
I (12352) http_server: Reverse search failed, trying forward search...
I (12353) http_server: Forward search found boundary at position 4096
I (12354) http_server: Clean file size: 4094 bytes (was 4567)
```

### If No Boundary Found:
```
I (12351) http_server: Searching for multipart boundary in 4567 bytes...
I (12352) http_server: Reverse search failed, trying forward search...
I (12353) http_server: No multipart boundary found, keeping original size: 4567 bytes
```

## Why This Should Work

### 1. Binary-Safe Search
- Uses `memcmp()` instead of `strstr()` for exact byte matching
- Won't be confused by binary data containing similar patterns

### 2. Reverse Search Priority
- Boundaries are always at the end of multipart data
- Searching backwards is more efficient and reliable

### 3. Robust Pattern Matching
- Looks for exact "------" + "WebKitFormBoundary" sequence
- Handles any random suffix after "WebKitFormBoundary"

### 4. Line-Aware Cleanup
- Finds the actual start of the boundary line
- Removes entire line, not just from boundary position

### 5. Comprehensive Fallback
- If reverse search fails, tries forward search
- If both fail, keeps original file (safe fallback)

## Test Scenarios

### Scenario 1: Normal Case
```
<4096 bytes of binary data>
------WebKitFormBoundaryA7wzDBy3Pl7Um1Nk--
```
**Expected**: Cut at position 4096, final size = 4096 bytes

### Scenario 2: With Newlines
```
<4094 bytes of binary data>\r\n------WebKitFormBoundaryA7wzDBy3Pl7Um1Nk--
```
**Expected**: Cut at position 4094, final size = 4094 bytes

### Scenario 3: No Newlines
```
<4096 bytes of binary data>------WebKitFormBoundaryA7wzDBy3Pl7Um1Nk--
```
**Expected**: Cut at position 4096, final size = 4096 bytes

## Verification Steps

1. **Upload .bin file**
2. **Check logs for boundary detection**
3. **Verify clean file size is reasonable**
4. **Check debug output shows binary data only**
5. **Download and verify UART output**

## If This Still Fails

Add this debug code to see exact file end:

```c
// Show last 100 bytes before cleanup
ESP_LOGI(TAG, "Last 100 bytes before cleanup:");
for (long i = raw_file_size - 100; i < raw_file_size; i++) {
    if (file_content[i] >= 32 && file_content[i] <= 126) {
        printf("%c", file_content[i]);
    } else {
        printf("[%02X]", (uint8_t)file_content[i]);
    }
}
printf("\n");
```

This will show exactly what the algorithm is working with.
