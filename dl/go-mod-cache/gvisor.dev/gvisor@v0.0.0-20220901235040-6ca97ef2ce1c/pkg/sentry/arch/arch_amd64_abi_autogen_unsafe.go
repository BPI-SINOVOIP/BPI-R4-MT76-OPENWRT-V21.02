// Automatically generated marshal implementation. See tools/go_marshal.

// If there are issues with build constraint aggregation, see
// tools/go_marshal/gomarshal/generator.go:writeHeader(). The constraints here
// come from the input set of files used to generate this file. This input set
// is filtered based on pre-defined file suffixes related to build constraints,
// see tools/defs.bzl:calculate_sets().

//go:build amd64 && amd64 && amd64
// +build amd64,amd64,amd64

package arch

import (
    "gvisor.dev/gvisor/pkg/abi/linux"
    "gvisor.dev/gvisor/pkg/gohacks"
    "gvisor.dev/gvisor/pkg/hostarch"
    "gvisor.dev/gvisor/pkg/marshal"
    "io"
    "reflect"
    "runtime"
    "unsafe"
)

// Marshallable types used by this file.
var _ marshal.Marshallable = (*FPSoftwareFrame)(nil)
var _ marshal.Marshallable = (*SignalContext64)(nil)
var _ marshal.Marshallable = (*UContext64)(nil)
var _ marshal.Marshallable = (*linux.SignalSet)(nil)
var _ marshal.Marshallable = (*linux.SignalStack)(nil)

// SizeBytes implements marshal.Marshallable.SizeBytes.
func (f *FPSoftwareFrame) SizeBytes() int {
    return 20 +
        4*7
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (f *FPSoftwareFrame) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(f.Magic1))
    dst = dst[4:]
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(f.ExtendedSize))
    dst = dst[4:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(f.Xfeatures))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint32(dst[:4], uint32(f.XstateSize))
    dst = dst[4:]
    for idx := 0; idx < 7; idx++ {
        hostarch.ByteOrder.PutUint32(dst[:4], uint32(f.Padding[idx]))
        dst = dst[4:]
    }
    return dst
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (f *FPSoftwareFrame) UnmarshalBytes(src []byte) []byte {
    f.Magic1 = uint32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    f.ExtendedSize = uint32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    f.Xfeatures = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    f.XstateSize = uint32(hostarch.ByteOrder.Uint32(src[:4]))
    src = src[4:]
    for idx := 0; idx < 7; idx++ {
        f.Padding[idx] = uint32(hostarch.ByteOrder.Uint32(src[:4]))
        src = src[4:]
    }
    return src
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (f *FPSoftwareFrame) Packed() bool {
    return true
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (f *FPSoftwareFrame) MarshalUnsafe(dst []byte) []byte {
    size := f.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(f), uintptr(size))
    return dst[size:]
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (f *FPSoftwareFrame) UnmarshalUnsafe(src []byte) []byte {
    size := f.SizeBytes()
    gohacks.Memmove(unsafe.Pointer(f), unsafe.Pointer(&src[0]), uintptr(size))
    return src[size:]
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (f *FPSoftwareFrame) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(f)))
    hdr.Len = f.SizeBytes()
    hdr.Cap = f.SizeBytes()

    length, err := cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that f
    // must live until the use above.
    runtime.KeepAlive(f) // escapes: replaced by intrinsic.
    return length, err
}

// CopyOut implements marshal.Marshallable.CopyOut.
func (f *FPSoftwareFrame) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return f.CopyOutN(cc, addr, f.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (f *FPSoftwareFrame) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(f)))
    hdr.Len = f.SizeBytes()
    hdr.Cap = f.SizeBytes()

    length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
    // Since we bypassed the compiler's escape analysis, indicate that f
    // must live until the use above.
    runtime.KeepAlive(f) // escapes: replaced by intrinsic.
    return length, err
}

// WriteTo implements io.WriterTo.WriteTo.
func (f *FPSoftwareFrame) WriteTo(writer io.Writer) (int64, error) {
    // Construct a slice backed by dst's underlying memory.
    var buf []byte
    hdr := (*reflect.SliceHeader)(unsafe.Pointer(&buf))
    hdr.Data = uintptr(gohacks.Noescape(unsafe.Pointer(f)))
    hdr.Len = f.SizeBytes()
    hdr.Cap = f.SizeBytes()

    length, err := writer.Write(buf)
    // Since we bypassed the compiler's escape analysis, indicate that f
    // must live until the use above.
    runtime.KeepAlive(f) // escapes: replaced by intrinsic.
    return int64(length), err
}

// SizeBytes implements marshal.Marshallable.SizeBytes.
func (s *SignalContext64) SizeBytes() int {
    return 184 +
        (*linux.SignalSet)(nil).SizeBytes() +
        8*8
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (s *SignalContext64) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.R8))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.R9))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.R10))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.R11))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.R12))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.R13))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.R14))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.R15))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rdi))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rsi))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rbp))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rbx))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rdx))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rax))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rcx))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rsp))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Rip))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Eflags))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint16(dst[:2], uint16(s.Cs))
    dst = dst[2:]
    hostarch.ByteOrder.PutUint16(dst[:2], uint16(s.Gs))
    dst = dst[2:]
    hostarch.ByteOrder.PutUint16(dst[:2], uint16(s.Fs))
    dst = dst[2:]
    hostarch.ByteOrder.PutUint16(dst[:2], uint16(s.Ss))
    dst = dst[2:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Err))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Trapno))
    dst = dst[8:]
    dst = s.Oldmask.MarshalUnsafe(dst)
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Cr2))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Fpstate))
    dst = dst[8:]
    for idx := 0; idx < 8; idx++ {
        hostarch.ByteOrder.PutUint64(dst[:8], uint64(s.Reserved[idx]))
        dst = dst[8:]
    }
    return dst
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (s *SignalContext64) UnmarshalBytes(src []byte) []byte {
    s.R8 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.R9 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.R10 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.R11 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.R12 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.R13 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.R14 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.R15 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rdi = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rsi = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rbp = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rbx = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rdx = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rax = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rcx = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rsp = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Rip = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Eflags = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Cs = uint16(hostarch.ByteOrder.Uint16(src[:2]))
    src = src[2:]
    s.Gs = uint16(hostarch.ByteOrder.Uint16(src[:2]))
    src = src[2:]
    s.Fs = uint16(hostarch.ByteOrder.Uint16(src[:2]))
    src = src[2:]
    s.Ss = uint16(hostarch.ByteOrder.Uint16(src[:2]))
    src = src[2:]
    s.Err = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Trapno = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    src = s.Oldmask.UnmarshalUnsafe(src)
    s.Cr2 = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    s.Fpstate = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    for idx := 0; idx < 8; idx++ {
        s.Reserved[idx] = uint64(hostarch.ByteOrder.Uint64(src[:8]))
        src = src[8:]
    }
    return src
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (s *SignalContext64) Packed() bool {
    return s.Oldmask.Packed()
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (s *SignalContext64) MarshalUnsafe(dst []byte) []byte {
    if s.Oldmask.Packed() {
        size := s.SizeBytes()
        gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(s), uintptr(size))
        return dst[size:]
    }
    // Type SignalContext64 doesn't have a packed layout in memory, fallback to MarshalBytes.
    return s.MarshalBytes(dst)
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (s *SignalContext64) UnmarshalUnsafe(src []byte) []byte {
    if s.Oldmask.Packed() {
        size := s.SizeBytes()
        gohacks.Memmove(unsafe.Pointer(s), unsafe.Pointer(&src[0]), uintptr(size))
        return src[size:]
    }
    // Type SignalContext64 doesn't have a packed layout in memory, fallback to UnmarshalBytes.
    return s.UnmarshalBytes(src)
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (s *SignalContext64) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    if !s.Oldmask.Packed() {
        // Type SignalContext64 doesn't have a packed layout in memory, fall back to MarshalBytes.
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
func (s *SignalContext64) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return s.CopyOutN(cc, addr, s.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (s *SignalContext64) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    if !s.Oldmask.Packed() {
        // Type SignalContext64 doesn't have a packed layout in memory, fall back to UnmarshalBytes.
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
func (s *SignalContext64) WriteTo(writer io.Writer) (int64, error) {
    if !s.Oldmask.Packed() {
        // Type SignalContext64 doesn't have a packed layout in memory, fall back to MarshalBytes.
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
func (u *UContext64) SizeBytes() int {
    return 16 +
        (*linux.SignalStack)(nil).SizeBytes() +
        (*SignalContext64)(nil).SizeBytes() +
        (*linux.SignalSet)(nil).SizeBytes()
}

// MarshalBytes implements marshal.Marshallable.MarshalBytes.
func (u *UContext64) MarshalBytes(dst []byte) []byte {
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(u.Flags))
    dst = dst[8:]
    hostarch.ByteOrder.PutUint64(dst[:8], uint64(u.Link))
    dst = dst[8:]
    dst = u.Stack.MarshalUnsafe(dst)
    dst = u.MContext.MarshalUnsafe(dst)
    dst = u.Sigset.MarshalUnsafe(dst)
    return dst
}

// UnmarshalBytes implements marshal.Marshallable.UnmarshalBytes.
func (u *UContext64) UnmarshalBytes(src []byte) []byte {
    u.Flags = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    u.Link = uint64(hostarch.ByteOrder.Uint64(src[:8]))
    src = src[8:]
    src = u.Stack.UnmarshalUnsafe(src)
    src = u.MContext.UnmarshalUnsafe(src)
    src = u.Sigset.UnmarshalUnsafe(src)
    return src
}

// Packed implements marshal.Marshallable.Packed.
//go:nosplit
func (u *UContext64) Packed() bool {
    return u.MContext.Packed() && u.Sigset.Packed() && u.Stack.Packed()
}

// MarshalUnsafe implements marshal.Marshallable.MarshalUnsafe.
func (u *UContext64) MarshalUnsafe(dst []byte) []byte {
    if u.MContext.Packed() && u.Sigset.Packed() && u.Stack.Packed() {
        size := u.SizeBytes()
        gohacks.Memmove(unsafe.Pointer(&dst[0]), unsafe.Pointer(u), uintptr(size))
        return dst[size:]
    }
    // Type UContext64 doesn't have a packed layout in memory, fallback to MarshalBytes.
    return u.MarshalBytes(dst)
}

// UnmarshalUnsafe implements marshal.Marshallable.UnmarshalUnsafe.
func (u *UContext64) UnmarshalUnsafe(src []byte) []byte {
    if u.MContext.Packed() && u.Sigset.Packed() && u.Stack.Packed() {
        size := u.SizeBytes()
        gohacks.Memmove(unsafe.Pointer(u), unsafe.Pointer(&src[0]), uintptr(size))
        return src[size:]
    }
    // Type UContext64 doesn't have a packed layout in memory, fallback to UnmarshalBytes.
    return u.UnmarshalBytes(src)
}

// CopyOutN implements marshal.Marshallable.CopyOutN.
func (u *UContext64) CopyOutN(cc marshal.CopyContext, addr hostarch.Addr, limit int) (int, error) {
    if !u.MContext.Packed() && u.Sigset.Packed() && u.Stack.Packed() {
        // Type UContext64 doesn't have a packed layout in memory, fall back to MarshalBytes.
        buf := cc.CopyScratchBuffer(u.SizeBytes()) // escapes: okay.
        u.MarshalBytes(buf) // escapes: fallback.
        return cc.CopyOutBytes(addr, buf[:limit]) // escapes: okay.
    }

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
func (u *UContext64) CopyOut(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    return u.CopyOutN(cc, addr, u.SizeBytes())
}

// CopyIn implements marshal.Marshallable.CopyIn.
func (u *UContext64) CopyIn(cc marshal.CopyContext, addr hostarch.Addr) (int, error) {
    if !u.MContext.Packed() && u.Sigset.Packed() && u.Stack.Packed() {
        // Type UContext64 doesn't have a packed layout in memory, fall back to UnmarshalBytes.
        buf := cc.CopyScratchBuffer(u.SizeBytes()) // escapes: okay.
        length, err := cc.CopyInBytes(addr, buf) // escapes: okay.
        // Unmarshal unconditionally. If we had a short copy-in, this results in a
        // partially unmarshalled struct.
        u.UnmarshalBytes(buf) // escapes: fallback.
        return length, err
    }

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
func (u *UContext64) WriteTo(writer io.Writer) (int64, error) {
    if !u.MContext.Packed() && u.Sigset.Packed() && u.Stack.Packed() {
        // Type UContext64 doesn't have a packed layout in memory, fall back to MarshalBytes.
        buf := make([]byte, u.SizeBytes())
        u.MarshalBytes(buf)
        length, err := writer.Write(buf)
        return int64(length), err
    }

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

