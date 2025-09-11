# Sửa lỗi Multipart Form Data và Protocol Commands

## Vấn đề ban đầu

### 1. File firmware bị dính header HTTP multipart:
```
------WebKitFormBoundaryf667xPOTCHKGkZhf
Content-Disposition: form-data; name="firmware"; filename="blink.hex"
Content-Type: application/octet-stream

:020000040800F2
:100000000050002059030008F5020008FD02000816
...
------WebKitFormBoundaryf667xPOTCHKGkZhf--
```

### 2. File .bin vẫn còn boundary ở cuối:
```
Ld��B�p�U�;�G6��U�;�������FpG������FpG
------WebKitFormBoundaryTGBfpYXV0cfzRbTw--
```

### 3. UART output có thêm protocol commands:
```
SIZE:13304
START
<firmware content>
END
```

## Giải pháp đã implement

### 1. Upload Handler - Cleanup hoàn toàn
```c
// Tìm và loại bỏ nhiều pattern boundary khác nhau
end_boundary = strstr(file_content, "\r\n------WebKitFormBoundary");
if (end_boundary == NULL) {
    end_boundary = strstr(file_content, "\n------WebKitFormBoundary");
}
if (end_boundary == NULL) {
    end_boundary = strstr(file_content, "------WebKitFormBoundary");
}

// Loại bỏ trailing whitespace và control characters
while (clean_file_size > 0 &&
       (file_content[clean_file_size - 1] == '\r' ||
        file_content[clean_file_size - 1] == '\n' ||
        file_content[clean_file_size - 1] == '\0' ||
        file_content[clean_file_size - 1] < 32)) {
    clean_file_size--;
}
```

### 2. Download Handler - Chỉ gửi nội dung file
```c
// Loại bỏ hoàn toàn:
// - SIZE:xxxx command
// - START command
// - END command

// Chỉ gửi firmware content qua UART
while (!feof(file)) {
    size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
    if (bytes_read > 0) {
        uart_write_bytes(UART_PORT_NUM, (const char*)buffer, bytes_read);
        // ... progress tracking
    }
}
```

## Kết quả

### Upload (SPIFFS file):
**Trước:**
```
------WebKitFormBoundary...
Content-Disposition: form-data...

:020000040800F2
:100000000050002059030008F5020008FD02000816
...
------WebKitFormBoundaryf667xPOTCHKGkZhf--
```

**Sau:**
```
:020000040800F2
:100000000050002059030008F5020008FD02000816
:10001000050300080D030008150300080000000098
...
:00000001FF
```

### Download (UART output):
**Trước:**
```
SIZE:13304
START
:020000040800F2
:100000000050002059030008F5020008FD02000816
...
END
```

**Sau:**
```
:020000040800F2
:100000000050002059030008F5020008FD02000816
:10001000050300080D030008150300080000000098
...
:00000001FF
```

## Tính năng

✅ **Upload Handler:**
- Loại bỏ hoàn toàn multipart boundaries
- Xử lý nhiều pattern boundary khác nhau
- Loại bỏ trailing whitespace/control chars
- Memory efficient streaming
- Progress tracking

✅ **Download Handler:**
- Chỉ gửi nội dung firmware thuần túy
- Không có protocol commands (SIZE/START/END)
- Truyền trực tiếp qua UART
- Progress tracking

✅ **Hỗ trợ:**
- File .hex và .bin
- Large files
- Error handling đầy đủ
- Detailed logging

## STM32 Bootloader Requirements

Giờ STM32 bootloader chỉ cần:
1. **Nhận firmware data trực tiếp** qua UART
2. **Không cần xử lý protocol commands** (SIZE/START/END)
3. **Detect end of transmission** bằng cách khác (timeout, file size, etc.)

## Test
1. Upload file .hex/.bin qua web interface
2. Kiểm tra file trong SPIFFS - sạch hoàn toàn
3. Download đến STM32 - chỉ firmware content
4. Kiểm tra UART output - không có protocol commands
