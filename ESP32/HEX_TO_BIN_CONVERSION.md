# HEX to BIN Conversion Feature

## Tổng quan
Tính năng tự động chuyển đổi file Intel HEX (.hex) sang Binary (.bin) khi upload firmware.

## Lý do implement
1. **Tối ưu truyền dữ liệu**: File .bin nhỏ hơn file .hex (không có ASCII overhead)
2. **Đơn giản hóa STM32**: STM32 bootloader chỉ cần xử lý binary data
3. **Tăng tốc độ**: Truyền binary nhanh hơn ASCII hex
4. **Tiết kiệm băng thông**: UART transmission hiệu quả hơn

## Workflow

### 1. Upload Process
```
User uploads .hex file
        ↓
ESP32 receives multipart data
        ↓
Clean multipart boundaries
        ↓
Detect file type (.hex or .bin)
        ↓
If .hex: Convert to .bin
        ↓
Store .bin in SPIFFS
```

### 2. Detection Logic
```c
// Method 1: Check filename extension
if (strncmp(ext_pos, ".hex", 4) == 0 || strncmp(ext_pos, ".HEX", 4) == 0) {
    is_hex_file = true;
}

// Method 2: Check file content
if (first_line[0] == ':') {  // Intel HEX starts with ':'
    is_hex_file = true;
}
```

### 3. Conversion Algorithm
```c
// Intel HEX Record Format: :LLAAAATT[DD...]CC
// LL = Data length
// AAAA = Address
// TT = Record type (00=data, 01=EOF, 04=extended address)
// DD = Data bytes
// CC = Checksum

// Two-pass conversion:
// Pass 1: Find maximum address → determine binary size
// Pass 2: Extract data and place at correct addresses
```

## Intel HEX Support

### Supported Record Types
- **00**: Data Record - Contains actual firmware data
- **01**: End of File Record - Marks end of hex file
- **04**: Extended Linear Address - For addresses > 64KB

### Example HEX Record
```
:10010000214601360121470136007EFE09D21940F7
```
- `10` = 16 bytes of data
- `0100` = Address 0x0100
- `00` = Data record
- `21460136...` = 16 bytes of data
- `F7` = Checksum

## File Size Comparison

### Example: STM32 Blink Program
- **HEX file**: 13,304 bytes (ASCII format)
- **BIN file**: 4,096 bytes (binary format)
- **Compression**: ~69% size reduction

### UART Transmission Time (115200 baud)
- **HEX**: ~1.16 seconds
- **BIN**: ~0.36 seconds
- **Speed improvement**: ~3.2x faster

## Implementation Details

### Memory Management
```c
// Allocate memory for binary data
binary_data = calloc(binary_size, 1);
memset(binary_data, 0xFF, binary_size);  // Initialize with 0xFF

// Process HEX records and fill binary data
// Write binary file
// Free memory
```

### Error Handling
- Invalid HEX format detection
- Memory allocation failures
- File I/O errors
- Checksum validation (future enhancement)

### File Operations
```c
// Temporary file handling
rename(FIRMWARE_FILE_PATH, TEMP_HEX_FILE_PATH);  // Backup original
convert_hex_to_bin(TEMP_HEX_FILE_PATH, FIRMWARE_FILE_PATH);  // Convert
remove(TEMP_HEX_FILE_PATH);  // Cleanup
```

## User Experience

### Upload .hex file
1. User selects .hex file
2. Upload progress shows
3. Conversion happens automatically
4. Success message: "HEX file uploaded and converted to BIN successfully"

### Upload .bin file
1. User selects .bin file
2. Upload progress shows
3. No conversion needed
4. Success message: "BIN file uploaded successfully"

### Download to STM32
- Always sends binary data (regardless of original format)
- Optimal transmission speed
- STM32 receives pure binary firmware

## Benefits

### For ESP32
- ✅ Unified storage format (.bin)
- ✅ Smaller SPIFFS usage
- ✅ Faster UART transmission
- ✅ Automatic format handling

### For STM32
- ✅ Simple binary data processing
- ✅ No HEX parsing required
- ✅ Faster firmware reception
- ✅ Direct flash programming

### For Users
- ✅ Upload any format (.hex or .bin)
- ✅ Automatic optimization
- ✅ Faster firmware updates
- ✅ Transparent conversion

## Future Enhancements

🔄 **Checksum Validation**: Verify HEX record checksums
🔄 **Compression**: Add firmware compression
🔄 **Multiple Formats**: Support Motorola S-record
🔄 **Metadata**: Store original filename and format info
🔄 **Progress**: Show conversion progress for large files

## Testing

### Test Cases
1. **Upload .hex file** → Should convert to .bin
2. **Upload .bin file** → Should store as-is
3. **Invalid .hex file** → Should show error
4. **Large .hex file** → Should handle memory efficiently
5. **Download after conversion** → Should send binary data

### Verification
```bash
# Check file sizes in SPIFFS
# Monitor conversion logs
# Verify UART output is binary
# Test STM32 firmware reception
```
