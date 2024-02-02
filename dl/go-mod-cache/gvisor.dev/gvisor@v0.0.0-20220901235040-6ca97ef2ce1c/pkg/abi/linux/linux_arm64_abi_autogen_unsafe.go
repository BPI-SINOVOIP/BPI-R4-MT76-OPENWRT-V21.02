// Automatically generated marshal implementation. See tools/go_marshal.

// If there are issues with build constraint aggregation, see
// tools/go_marshal/gomarshal/generator.go:writeHeader(). The constraints here
// come from the input set of files used to generate this file. This input set
// is filtered based on pre-defined file suffixes related to build constraints,
// see tools/defs.bzl:calculate_sets().

//go:build arm64 && arm64 && arm64 && arm64
// +build arm64,arm64,arm64,arm64

package linux

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
var _ marshal.Marshallable = (*EpollEvent)(nil)
var _ marshal.Marshallable = (*IPCPerm)(nil)
var _ marshal.Marshallable = (*PtraceRegs)(nil)
var _ marshal.Marshallable = (*SemidDS)(nil)
var _ marshal.Marshallable = (*Stat)(nil)
var _ marshal.Marshallable = (*TimeT)(nil)
var _ marshal.Marshallable = (*Timespec)(nil)

// SizeBytes implements marshal.Marshallable.SizeBytes.
func (e *EpollEvent) SizeBytes() int {
    return 8 +
        4*2
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (e *EpollEvent) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(e.Events))
    dst = dst[4:]
    // Padding: dst[:sizeof(int32)] ~= int32(0)
    dst = dst[4:]
    for idx := 0; idx < 2; idx++ {
        hostarch.ByteOrder.PutUint32(dst[:4], uint32(e.Data[idx]))
        dst = dst[4:]
    }
    return dst
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (e *EpollEvent) UnmarshalBytes(src []byte) []byte {
    e.Events = uint32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    // Padding: var _ int32 ~= src[:sizeof(int32)]
    src = src[4:]
    for idx := 0; idx < 2; idx++ {
        e.Data[idx] = int32(hostarch.ByteOrder.Uint32(src[:4]))
        src = src[4:]
    }
    return src
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (e *EpollEvent) Packed() bool {
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (e *EpollEvent) MarshalUnsafe(dst []byte) []byte {
    size := e.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(e), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (e *EpollEvent) UnmarshalUnsafe(src []byte) []byte {
    size := e.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(e), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (e *EpollEvent) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(e)))
    hdr.Len = e.SizeBytes()
    hdr.Cap = e.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that e
    // must live until the use above.
    runtime.KeepAlive(e) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (e *EpollEvent) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return e.CopyOutN(cc, addr, e.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (e *EpollEvent) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(e)))
    hdr.Len = e.SizeBytes()
    hdr.Cap = e.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that e
    // must live until the use above.
    runtime.KeepAlive(e) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (e *EpollEvent) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(e)))
    hdr.Len = e.SizeBytes()
    hdr.Cap = e.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that e
    // must live until the use above.
    runtime.KeepAlive(e) // escapes: replaced by intrinsic.
    return int64(length), err
}

// CopyEpollEventSliceIn copies in a slice of EpollEvent objects from the task's memory.
func CopyEpollEventSliceIn(cc marshal.CopyContext, addr hostarch.Addr, dst []EpollEvent) (int, error) {
    count := len(dst)
    if count == 0 {
        return 0, nil
    }
    size := (*EpollEvent)(nil).SizeBytes()

    ptr := unsafe.Pointer(&dst)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyInBytes(addr, buf)
    // Since we bypassed the compiler's escape analysis, indicate that dst
    // must live until the use above.
    runtime.KeepAlive(dst) // escapes: replaced by intrinsic.
    return length, err
}

// CopyEpollEventSliceOut copies a slice of EpollEvent objects to the task's memory.
func CopyEpollEventSliceOut(cc marshal.CopyContext, addr hostarch.Addr, src []EpollEvent) (int, error) {
    count := len(src)
    if count == 0 {
        return 0, nil
    }
    size := (*EpollEvent)(nil).SizeBytes()

    ptr := unsafe.Pointer(&src)
    val := gohacks.Noescape(unsafe.Pointer((*reflect.SliceHeader)(ptr).Data))

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(val)
    hdr.Len = size * count
    hdr.Cap = size * count

    length, err := cc.CopyOutBytes(addr, buf)
    // Since we bypassed the compiler's escape analysis, indicate that src
    // must live until the use above.
    runtime.KeepAlive(src) // escapes: replaced by intrinsic.
    return length, err
}

// MarshalUnsafeEpollEventSlice is like EpollEvent.MarshalUnsafe, but for a []EpollEvent.
func MarshalUnsafeEpollEventSlice(src []EpollEvent, dst []byte) []byte {
    count := len(src)
    if count == 0 {
        return dst
    }

    size := (*EpollEvent)(nil).SizeBytes()
    buf := dst[:size*count]
    gohacks.Memmove(unsafe.Pointer(&buf[0]), unsafe.Pointer(&src[0]), uintptr(len(buf)))
    return dst[size*count:]
}

// UnmarshalUnsafeEpollEventSlice is like EpollEvent.UnmarshalUnsafe, but for a []EpollEvent.
func UnmarshalUnsafeEpollEventSlice(dst []EpollEvent, src []byte) []byte {
    count := len(dst)
    if count == 0 {
        return src
    }

    size := (*EpollEvent)(nil).SizeBytes()
    buf := src[:size*count]
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(&buf[0]), uintptr(len(buf)))
    return src[size*count:]
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
func (s *Stat) SizeBytes() int {
    return 72 +
        (*Timespec)(nil).SizeBytes() +
        (*Timespec)(nil).SizeBytes() +
        (*Timespec)(nil).SizeBytes() +
        4*2
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (s *Stat) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Dev))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Ino))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(s.Mode))
    dst = dst[4:]
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(s.Nlink))
    dst = dst[4:]
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(s.UID))
    dst = dst[4:]
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(s.GID))
    dst = dst[4:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rdev))
    dst = dst[8:]
    // Padding: dst[:sizeof(uint64)] ~= uint64(0)
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Size))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(s.Blksize))
    dst = dst[4:]
    // Padding: dst[:sizeof(int32)] ~= int32(0)
    dst = dst[4:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Blocks))
    dst = dst[8:]
    dst = s.ATime.MarshalUnsafe(dst)
    dst = s.MTime.MarshalUnsafe(dst)
    dst = s.CTime.MarshalUnsafe(dst)
    // Padding: dst[:sizeof(int32)*2] ~= [2]int32{0}
    dst = dst[4*(2):]
    return dst
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (s *Stat) UnmarshalBytes(src []byte) []byte {
    s.Dev = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Ino = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Mode = uint32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    s.Nlink = uint32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    s.UID = uint32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    s.GID = uint32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    s.Rdev = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    // Padding: var _ uint64 ~= src[:sizeof(uint64)]
    src = src[8:]
    s.Size = int64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Blksize = int32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    // Padding: var _ int32 ~= src[:sizeof(int32)]
    src = src[4:]
    s.Blocks = int64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    src = s.ATime.UnmarshalUnsafe(src)
    src = s.MTime.UnmarshalUnsafe(src)
    src = s.CTime.UnmarshalUnsafe(src)
    // Padding: ~ copy([2]int32(s._), src[:sizeof(int32)*2])
    src = src[4*(2):]
    return src
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (s *Stat) Packed() bool {
    return s.ATime.Packed() && s.CTime.Packed() && s.MTime.Packed()
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (s *Stat) MarshalUnsafe(dst []byte) []byte {
    if s.ATime.Packed() && s.CTime.Packed() && s.MTime.Packed() {
        size := s.SizeBytes()
        gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(s), uintptr(size))
        return dst[size:]
    }
    // Type Stat doesn't have a packed layout in memory, fallback to MarshalBytes.
    return s.MarshalBytes(dst)
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (s *Stat) UnmarshalUnsafe(src []byte) []byte {
    if s.ATime.Packed() && s.CTime.Packed() && s.MTime.Packed() {
        size := s.SizeBytes()
        gohacks.Memmove(unsafe.Pointer(s), unsafe.Pointer(&src[0]), uintptr(size))
        return src[size:]
    }
    // Type Stat doesn't have a packed layout in memory, fallback to UnmarshalBytes.
    return s.UnmarshalBytes(src)
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (s *Stat) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    if !s.ATime.Packed() && s.CTime.Packed() && s.MTime.Packed() {
        // Type Stat doesn't have a packed layout in memory, fall back to MarshalBytes.
        buf := cc.CopyScratchBuffer(s.SizeBytes()) // escapes: okay.
        s.MarshalBytes(buf) // escapes: fallback.
        return cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    }

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(s)))
    hdr.Len = s.SizeBytes()
    hdr.Cap = s.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that s
    // must live until the use above.
    runtime.KeepAlive(s) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (s *Stat) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return s.CopyOutN(cc, addr, s.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (s *Stat) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    if !s.ATime.Packed() && s.CTime.Packed() && s.MTime.Packed() {
        // Type Stat doesn't have a packed layout in memory, fall back to UnmarshalBytes.
        buf := cc.CopyScratchBuffer(s.SizeBytes()) // escapes: okay.
        length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
        // Unmarshal unconditionally. If we had a short copy-in, this results in a
        // partially unmarshalled struct.
        s.UnmarshalBytes(buf) // escapes: fallback.
        return length, err
    }

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(s)))
    hdr.Len = s.SizeBytes()
    hdr.Cap = s.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that s
    // must live until the use above.
    runtime.KeepAlive(s) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (s *Stat) WriteTo(writer io.Writer) (int64, error) {
    if !s.ATime.Packed() && s.CTime.Packed() && s.MTime.Packed() {
        // Type Stat doesn't have a packed layout in memory, fall back to MarshalBytes.
        buf := make([]byte, s.SizeBytes())
        s.MarshalBytes(buf)
        length, err := writer.Write(buf)
        return int64(length), err
    }

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(s)))
    hdr.Len = s.SizeBytes()
    hdr.Cap = s.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that s
    // must live until the use above.
    runtime.KeepAlive(s) // escapes: replaced by intrinsic.
    return int64(length), err
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
func (p *PtraceRegs) SizeBytes() int {
    return 24 +
        8*31
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (p *PtraceRegs) MarshalBytes(dst []byte) []byte {
    for idx := 0; idx < 31; idx++ {
        hostarch.ByteOrder.PutUint64(dst[:8], uint64(p.Regs[idx]))
        dst = dst[8:]
    }
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(p.Sp))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(p.Pc))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(p.Pstate))
    dst = dst[8:]
    return dst
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (p *PtraceRegs) UnmarshalBytes(src []byte) []byte {
    for idx := 0; idx < 31; idx++ {
        p.Regs[idx] = uint64(hostarch.ByteOrder.Uint64(src[:8]))
        src = src[8:]
    }
    p.Sp = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    p.Pc = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    p.Pstate = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    return src
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (p *PtraceRegs) Packed() bool {
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (p *PtraceRegs) MarshalUnsafe(dst []byte) []byte {
    size := p.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(p), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (p *PtraceRegs) UnmarshalUnsafe(src []byte) []byte {
    size := p.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(p), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (p *PtraceRegs) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(p)))
    hdr.Len = p.SizeBytes()
    hdr.Cap = p.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that p
    // must live until the use above.
    runtime.KeepAlive(p) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (p *PtraceRegs) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return p.CopyOutN(cc, addr, p.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (p *PtraceRegs) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(p)))
    hdr.Len = p.SizeBytes()
    hdr.Cap = p.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that p
    // must live until the use above.
    runtime.KeepAlive(p) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (p *PtraceRegs) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(p)))
    hdr.Len = p.SizeBytes()
    hdr.Cap = p.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that p
    // must live until the use above.
    runtime.KeepAlive(p) // escapes: replaced by intrinsic.
    return int64(length), err
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
func (s *SemidDS) SizeBytes() int {
    return 24 +
        (*IPCPerm)(nil).SizeBytes() +
        (*TimeT)(nil).SizeBytes() +
        (*TimeT)(nil).SizeBytes()
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (s *SemidDS) MarshalBytes(dst []byte) []byte {
    dst = s.SemPerm.MarshalUnsafe(dst)
    dst = s.SemOTime.MarshalUnsafe(dst)
    dst = s.SemCTime.MarshalUnsafe(dst)
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.SemNSems))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.unused3))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.unused4))
    dst = dst[8:]
    return dst
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (s *SemidDS) UnmarshalBytes(src []byte) []byte {
    src = s.SemPerm.UnmarshalUnsafe(src)
    src = s.SemOTime.UnmarshalUnsafe(src)
    src = s.SemCTime.UnmarshalUnsafe(src)
    s.SemNSems = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.unused3 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.unused4 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    return src
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (s *SemidDS) Packed() bool {
    return s.SemCTime.Packed() && s.SemOTime.Packed() && s.SemPerm.Packed()
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (s *SemidDS) MarshalUnsafe(dst []byte) []byte {
    if s.SemCTime.Packed() && s.SemOTime.Packed() && s.SemPerm.Packed() {
        size := s.SizeBytes()
        gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(s), uintptr(size))
        return dst[size:]
    }
    // Type SemidDS doesn't have a packed layout in memory, fallback to MarshalBytes.
    return s.MarshalBytes(dst)
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (s *SemidDS) UnmarshalUnsafe(src []byte) []byte {
    if s.SemCTime.Packed() && s.SemOTime.Packed() && s.SemPerm.Packed() {
        size := s.SizeBytes()
        gohacks.Memmove(unsafe.Pointer(s), unsafe.Pointer(&src[0]), uintptr(size))
        return src[size:]
    }
    // Type SemidDS doesn't have a packed layout in memory, fallback to UnmarshalBytes.
    return s.UnmarshalBytes(src)
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (s *SemidDS) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    if !s.SemCTime.Packed() && s.SemOTime.Packed() && s.SemPerm.Packed() {
        // Type SemidDS doesn't have a packed layout in memory, fall back to MarshalBytes.
        buf := cc.CopyScratchBuffer(s.SizeBytes()) // escapes: okay.
        s.MarshalBytes(buf) // escapes: fallback.
        return cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    }

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(s)))
    hdr.Len = s.SizeBytes()
    hdr.Cap = s.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that s
    // must live until the use above.
    runtime.KeepAlive(s) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (s *SemidDS) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return s.CopyOutN(cc, addr, s.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (s *SemidDS) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    if !s.SemCTime.Packed() && s.SemOTime.Packed() && s.SemPerm.Packed() {
        // Type SemidDS doesn't have a packed layout in memory, fall back to UnmarshalBytes.
        buf := cc.CopyScratchBuffer(s.SizeBytes()) // escapes: okay.
        length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
        // Unmarshal unconditionally. If we had a short copy-in, this results in a
        // partially unmarshalled struct.
        s.UnmarshalBytes(buf) // escapes: fallback.
        return length, err
    }

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(s)))
    hdr.Len = s.SizeBytes()
    hdr.Cap = s.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that s
    // must live until the use above.
    runtime.KeepAlive(s) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (s *SemidDS) WriteTo(writer io.Writer) (int64, error) {
    if !s.SemCTime.Packed() && s.SemOTime.Packed() && s.SemPerm.Packed() {
        // Type SemidDS doesn't have a packed layout in memory, fall back to MarshalBytes.
        buf := make([]byte, s.SizeBytes())
        s.MarshalBytes(buf)
        length, err := writer.Write(buf)
        return int64(length), err
    }

    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(s)))
    hdr.Len = s.SizeBytes()
    hdr.Cap = s.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that s
    // must live until the use above.
    runtime.KeepAlive(s) // escapes: replaced by intrinsic.
    return int64(length), err
}

