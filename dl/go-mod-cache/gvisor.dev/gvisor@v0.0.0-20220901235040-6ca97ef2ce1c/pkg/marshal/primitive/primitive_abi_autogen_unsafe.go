// Automatically generated marshal implementation. See tools/go_marshal.

package primitive

import (
    "gvisor.dev/gvisor/pkg/gohacks"
    "gvisor.dev/gvisor/pkg/hostarch"
    "gvisor.dev/gvisor/pkg/marshal"
    "io"
    "reflect"
    "runtime"
    "unsafe"
)

// Marshallable types used by this file.
var _ marshal.Marshallable = (*Int16)(nil)
var _ marshal.Marshallable = (*Int32)(nil)
var _ marshal.Marshallable = (*Int64)(nil)
var _ marshal.Marshallable = (*Int8)(nil)
var _ marshal.Marshallable = (*Uint16)(nil)
var _ marshal.Marshallable = (*Uint32)(nil)
var _ marshal.Marshallable = (*Uint64)(nil)
var _ marshal.Marshallable = (*Uint8)(nil)

// SizeBytes implements marshal.Marshallable.SizeBytes.
//go:nosplit
func (i *Int16) SizeBytes() int {
    return 2
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (i *Int16) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint16(dst[:2], uint16(*i))
    return dst[2:]
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (i *Int16) UnmarshalBytes(src []byte) []byte {
    *i = Int16(int16(hostarch.ByteOrder.Uint16(src[:2])))
    return src[2:]
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (i *Int16) Packed() bool {
    // Scalar newtypes are always packed.
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (i *Int16) MarshalUnsafe(dst []byte) []byte {
    size := i.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(i), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (i *Int16) UnmarshalUnsafe(src []byte) []byte {
    size := i.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(i), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (i *Int16) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (i *Int16) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return i.CopyOutN(cc, addr, i.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (i *Int16) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (i *Int16) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CheckedMarshal implements marshal.CheckedMarshallable.CheckedMarshal.
func (i *Int16) CheckedMarshal(dst []byte) ([]byte, bool) {
    size := i.SizeBytes()
    if size > len(dst) {
        return dst, false
    }
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(i), uintptr(size))
    return dst[size:], true
}

// CheckedUnmarshal implements marshal.CheckedMarshallable.CheckedUnmarshal.
func (i *Int16) CheckedUnmarshal(src []byte) ([]byte, bool) {
    size := i.SizeBytes()
    if size > len(src) {
        return src, false
    }
    gohacks.Memmove(unsafe.Pointer(i), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:], true
}

// CopyInt16SliceIn copies in a slice of int16 objects from the task's memory.
func CopyInt16SliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []int16) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*Int16)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyInt16SliceOut copies a slice of int16 objects to the task's memory.
func CopyInt16SliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []int16) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*Int16)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeInt16Slice is like Int16.MarshalUnsafe, but for a []Int16.
func MarshalUnsafeInt16Slice(src []Int16, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }
    size := (*Int16)(nil).SizeBytes()

    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeInt16Slice is like Int16.UnmarshalUnsafe, but for a []Int16.
func UnmarshalUnsafeInt16Slice(dst []Int16, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }
    size := (*Int16)(nil).SizeBytes()

    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
//go:nosplit
func (i *Int32) SizeBytes() int {
    return 4
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (i *Int32) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(*i))
    return dst[4:]
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (i *Int32) UnmarshalBytes(src []byte) []byte {
    *i = Int32(int32(hostarch.ByteOrder.Uint32(src[:4])))
    return src[4:]
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (i *Int32) Packed() bool {
    // Scalar newtypes are always packed.
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (i *Int32) MarshalUnsafe(dst []byte) []byte {
    size := i.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(i), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (i *Int32) UnmarshalUnsafe(src []byte) []byte {
    size := i.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(i), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (i *Int32) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (i *Int32) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return i.CopyOutN(cc, addr, i.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (i *Int32) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (i *Int32) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CheckedMarshal implements marshal.CheckedMarshallable.CheckedMarshal.
func (i *Int32) CheckedMarshal(dst []byte) ([]byte, bool) {
    size := i.SizeBytes()
    if size > len(dst) {
        return dst, false
    }
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(i), uintptr(size))
    return dst[size:], true
}

// CheckedUnmarshal implements marshal.CheckedMarshallable.CheckedUnmarshal.
func (i *Int32) CheckedUnmarshal(src []byte) ([]byte, bool) {
    size := i.SizeBytes()
    if size > len(src) {
        return src, false
    }
    gohacks.Memmove(unsafe.Pointer(i), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:], true
}

// CopyInt32SliceIn copies in a slice of int32 objects from the task's memory.
func CopyInt32SliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []int32) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*Int32)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyInt32SliceOut copies a slice of int32 objects to the task's memory.
func CopyInt32SliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []int32) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*Int32)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeInt32Slice is like Int32.MarshalUnsafe, but for a []Int32.
func MarshalUnsafeInt32Slice(src []Int32, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }
    size := (*Int32)(nil).SizeBytes()

    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeInt32Slice is like Int32.UnmarshalUnsafe, but for a []Int32.
func UnmarshalUnsafeInt32Slice(dst []Int32, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }
    size := (*Int32)(nil).SizeBytes()

    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
//go:nosplit
func (i *Int64) SizeBytes() int {
    return 8
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (i *Int64) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(*i))
    return dst[8:]
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (i *Int64) UnmarshalBytes(src []byte) []byte {
    *i = Int64(int64(hostarch.ByteOrder.Uint64(src[:8])))
    return src[8:]
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (i *Int64) Packed() bool {
    // Scalar newtypes are always packed.
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (i *Int64) MarshalUnsafe(dst []byte) []byte {
    size := i.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(i), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (i *Int64) UnmarshalUnsafe(src []byte) []byte {
    size := i.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(i), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (i *Int64) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (i *Int64) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return i.CopyOutN(cc, addr, i.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (i *Int64) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (i *Int64) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CheckedMarshal implements marshal.CheckedMarshallable.CheckedMarshal.
func (i *Int64) CheckedMarshal(dst []byte) ([]byte, bool) {
    size := i.SizeBytes()
    if size > len(dst) {
        return dst, false
    }
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(i), uintptr(size))
    return dst[size:], true
}

// CheckedUnmarshal implements marshal.CheckedMarshallable.CheckedUnmarshal.
func (i *Int64) CheckedUnmarshal(src []byte) ([]byte, bool) {
    size := i.SizeBytes()
    if size > len(src) {
        return src, false
    }
    gohacks.Memmove(unsafe.Pointer(i), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:], true
}

// CopyInt64SliceIn copies in a slice of int64 objects from the task's memory.
func CopyInt64SliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []int64) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*Int64)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyInt64SliceOut copies a slice of int64 objects to the task's memory.
func CopyInt64SliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []int64) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*Int64)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeInt64Slice is like Int64.MarshalUnsafe, but for a []Int64.
func MarshalUnsafeInt64Slice(src []Int64, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }
    size := (*Int64)(nil).SizeBytes()

    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeInt64Slice is like Int64.UnmarshalUnsafe, but for a []Int64.
func UnmarshalUnsafeInt64Slice(dst []Int64, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }
    size := (*Int64)(nil).SizeBytes()

    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
//go:nosplit
func (i *Int8) SizeBytes() int {
    return 1
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (i *Int8) MarshalBytes(dst []byte) []byte {
    dst[0] = byte(*i)
    return dst[1:]
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (i *Int8) UnmarshalBytes(src []byte) []byte {
    *i = Int8(int8(src[0]))
    return src[1:]
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (i *Int8) Packed() bool {
    // Scalar newtypes are always packed.
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (i *Int8) MarshalUnsafe(dst []byte) []byte {
    size := i.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(i), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (i *Int8) UnmarshalUnsafe(src []byte) []byte {
    size := i.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(i), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (i *Int8) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (i *Int8) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return i.CopyOutN(cc, addr, i.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (i *Int8) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (i *Int8) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(i)))
    hdr.Len = i.SizeBytes()
    hdr.Cap = i.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that i
    // must live until the use above.
    runtime.KeepAlive(i) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CheckedMarshal implements marshal.CheckedMarshallable.CheckedMarshal.
func (i *Int8) CheckedMarshal(dst []byte) ([]byte, bool) {
    size := i.SizeBytes()
    if size > len(dst) {
        return dst, false
    }
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(i), uintptr(size))
    return dst[size:], true
}

// CheckedUnmarshal implements marshal.CheckedMarshallable.CheckedUnmarshal.
func (i *Int8) CheckedUnmarshal(src []byte) ([]byte, bool) {
    size := i.SizeBytes()
    if size > len(src) {
        return src, false
    }
    gohacks.Memmove(unsafe.Pointer(i), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:], true
}

// CopyInt8SliceIn copies in a slice of int8 objects from the task's memory.
func CopyInt8SliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []int8) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*Int8)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyInt8SliceOut copies a slice of int8 objects to the task's memory.
func CopyInt8SliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []int8) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*Int8)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeInt8Slice is like Int8.MarshalUnsafe, but for a []Int8.
func MarshalUnsafeInt8Slice(src []Int8, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }
    size := (*Int8)(nil).SizeBytes()

    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeInt8Slice is like Int8.UnmarshalUnsafe, but for a []Int8.
func UnmarshalUnsafeInt8Slice(dst []Int8, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }
    size := (*Int8)(nil).SizeBytes()

    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
//go:nosplit
func (u *Uint16) SizeBytes() int {
    return 2
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (u *Uint16) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint16(dst[:2], uint16(*u))
    return dst[2:]
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (u *Uint16) UnmarshalBytes(src []byte) []byte {
    *u = Uint16(uint16(hostarch.ByteOrder.Uint16(src[:2])))
    return src[2:]
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (u *Uint16) Packed() bool {
    // Scalar newtypes are always packed.
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (u *Uint16) MarshalUnsafe(dst []byte) []byte {
    size := u.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (u *Uint16) UnmarshalUnsafe(src []byte) []byte {
    size := u.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (u *Uint16) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (u *Uint16) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return u.CopyOutN(cc, addr, u.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (u *Uint16) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (u *Uint16) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CheckedMarshal implements marshal.CheckedMarshallable.CheckedMarshal.
func (u *Uint16) CheckedMarshal(dst []byte) ([]byte, bool) {
    size := u.SizeBytes()
    if size > len(dst) {
        return dst, false
    }
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
    return dst[size:], true
}

// CheckedUnmarshal implements marshal.CheckedMarshallable.CheckedUnmarshal.
func (u *Uint16) CheckedUnmarshal(src []byte) ([]byte, bool) {
    size := u.SizeBytes()
    if size > len(src) {
        return src, false
    }
    gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:], true
}

// CopyUint16SliceIn copies in a slice of uint16 objects from the task's memory.
func CopyUint16SliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []uint16) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*Uint16)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyUint16SliceOut copies a slice of uint16 objects to the task's memory.
func CopyUint16SliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []uint16) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*Uint16)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeUint16Slice is like Uint16.MarshalUnsafe, but for a []Uint16.
func MarshalUnsafeUint16Slice(src []Uint16, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }
    size := (*Uint16)(nil).SizeBytes()

    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeUint16Slice is like Uint16.UnmarshalUnsafe, but for a []Uint16.
func UnmarshalUnsafeUint16Slice(dst []Uint16, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }
    size := (*Uint16)(nil).SizeBytes()

    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
//go:nosplit
func (u *Uint32) SizeBytes() int {
    return 4
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (u *Uint32) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(*u))
    return dst[4:]
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (u *Uint32) UnmarshalBytes(src []byte) []byte {
    *u = Uint32(uint32(hostarch.ByteOrder.Uint32(src[:4])))
    return src[4:]
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (u *Uint32) Packed() bool {
    // Scalar newtypes are always packed.
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (u *Uint32) MarshalUnsafe(dst []byte) []byte {
    size := u.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (u *Uint32) UnmarshalUnsafe(src []byte) []byte {
    size := u.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (u *Uint32) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (u *Uint32) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return u.CopyOutN(cc, addr, u.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (u *Uint32) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (u *Uint32) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CheckedMarshal implements marshal.CheckedMarshallable.CheckedMarshal.
func (u *Uint32) CheckedMarshal(dst []byte) ([]byte, bool) {
    size := u.SizeBytes()
    if size > len(dst) {
        return dst, false
    }
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
    return dst[size:], true
}

// CheckedUnmarshal implements marshal.CheckedMarshallable.CheckedUnmarshal.
func (u *Uint32) CheckedUnmarshal(src []byte) ([]byte, bool) {
    size := u.SizeBytes()
    if size > len(src) {
        return src, false
    }
    gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:], true
}

// CopyUint32SliceIn copies in a slice of uint32 objects from the task's memory.
func CopyUint32SliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []uint32) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*Uint32)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyUint32SliceOut copies a slice of uint32 objects to the task's memory.
func CopyUint32SliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []uint32) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*Uint32)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeUint32Slice is like Uint32.MarshalUnsafe, but for a []Uint32.
func MarshalUnsafeUint32Slice(src []Uint32, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }
    size := (*Uint32)(nil).SizeBytes()

    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeUint32Slice is like Uint32.UnmarshalUnsafe, but for a []Uint32.
func UnmarshalUnsafeUint32Slice(dst []Uint32, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }
    size := (*Uint32)(nil).SizeBytes()

    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
//go:nosplit
func (u *Uint64) SizeBytes() int {
    return 8
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (u *Uint64) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(*u))
    return dst[8:]
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (u *Uint64) UnmarshalBytes(src []byte) []byte {
    *u = Uint64(uint64(hostarch.ByteOrder.Uint64(src[:8])))
    return src[8:]
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (u *Uint64) Packed() bool {
    // Scalar newtypes are always packed.
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (u *Uint64) MarshalUnsafe(dst []byte) []byte {
    size := u.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (u *Uint64) UnmarshalUnsafe(src []byte) []byte {
    size := u.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (u *Uint64) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (u *Uint64) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return u.CopyOutN(cc, addr, u.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (u *Uint64) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (u *Uint64) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CheckedMarshal implements marshal.CheckedMarshallable.CheckedMarshal.
func (u *Uint64) CheckedMarshal(dst []byte) ([]byte, bool) {
    size := u.SizeBytes()
    if size > len(dst) {
        return dst, false
    }
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
    return dst[size:], true
}

// CheckedUnmarshal implements marshal.CheckedMarshallable.CheckedUnmarshal.
func (u *Uint64) CheckedUnmarshal(src []byte) ([]byte, bool) {
    size := u.SizeBytes()
    if size > len(src) {
        return src, false
    }
    gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:], true
}

// CopyUint64SliceIn copies in a slice of uint64 objects from the task's memory.
func CopyUint64SliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []uint64) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*Uint64)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyUint64SliceOut copies a slice of uint64 objects to the task's memory.
func CopyUint64SliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []uint64) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*Uint64)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeUint64Slice is like Uint64.MarshalUnsafe, but for a []Uint64.
func MarshalUnsafeUint64Slice(src []Uint64, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }
    size := (*Uint64)(nil).SizeBytes()

    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeUint64Slice is like Uint64.UnmarshalUnsafe, but for a []Uint64.
func UnmarshalUnsafeUint64Slice(dst []Uint64, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }
    size := (*Uint64)(nil).SizeBytes()

    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
//go:nosplit
func (u *Uint8) SizeBytes() int {
    return 1
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (u *Uint8) MarshalBytes(dst []byte) []byte {
    dst[0] = byte(*u)
    return dst[1:]
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (u *Uint8) UnmarshalBytes(src []byte) []byte {
    *u = Uint8(uint8(src[0]))
    return src[1:]
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (u *Uint8) Packed() bool {
    // Scalar newtypes are always packed.
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (u *Uint8) MarshalUnsafe(dst []byte) []byte {
    size := u.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (u *Uint8) UnmarshalUnsafe(src []byte) []byte {
    size := u.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (u *Uint8) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (u *Uint8) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return u.CopyOutN(cc, addr, u.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (u *Uint8) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (u *Uint8) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(u)))
    hdr.Len = u.SizeBytes()
    hdr.Cap = u.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that u
    // must live until the use above.
    runtime.KeepAlive(u) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CheckedMarshal implements marshal.CheckedMarshallable.CheckedMarshal.
func (u *Uint8) CheckedMarshal(dst []byte) ([]byte, bool) {
    size := u.SizeBytes()
    if size > len(dst) {
        return dst, false
    }
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
    return dst[size:], true
}

// CheckedUnmarshal implements marshal.CheckedMarshallable.CheckedUnmarshal.
func (u *Uint8) CheckedUnmarshal(src []byte) ([]byte, bool) {
    size := u.SizeBytes()
    if size > len(src) {
        return src, false
    }
    gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:], true
}

// CopyUint8SliceIn copies in a slice of uint8 objects from the task's memory.
func CopyUint8SliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []uint8) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*Uint8)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyUint8SliceOut copies a slice of uint8 objects to the task's memory.
func CopyUint8SliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []uint8) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*Uint8)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeUint8Slice is like Uint8.MarshalUnsafe, but for a []Uint8.
func MarshalUnsafeUint8Slice(src []Uint8, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }
    size := (*Uint8)(nil).SizeBytes()

    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeUint8Slice is like Uint8.UnmarshalUnsafe, but for a []Uint8.
func UnmarshalUnsafeUint8Slice(dst []Uint8, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }
    size := (*Uint8)(nil).SizeBytes()

    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

