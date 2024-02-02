package shadowaead_2022_test

import (
	"context"
	"crypto/rand"
	"net"
	"sync"
	"testing"

	"github.com/sagernet/sing-shadowsocks/shadowaead_2022"
	"github.com/sagernet/sing/common"
	E "github.com/sagernet/sing/common/exceptions"
	M "github.com/sagernet/sing/common/metadata"
)

func TestService(t *testing.T) {
	t.Parallel()
	method := "2022-blake3-aes-128-gcm"
	var psk [16]byte
	rand.Reader.Read(psk[:])

	var wg sync.WaitGroup

	service, err := shadowaead_2022.NewService(method, psk[:], 500, &multiHandler{t, &wg})
	if err != nil {
		t.Fatal(err)
	}

	client, err := shadowaead_2022.New(method, [][]byte{psk[:]})
	if err != nil {
		t.Fatal(err)
	}
	wg.Add(1)

	serverConn, clientConn := net.Pipe()
	defer common.Close(serverConn, clientConn)
	go func() {
		err := service.NewConnection(context.Background(), serverConn, M.Metadata{})
		if err != nil {
			serverConn.Close()
			t.Error(E.Cause(err, "server"))
			return
		}
	}()
	_, err = client.DialConn(clientConn, M.ParseSocksaddr("test.com:443"))
	if err != nil {
		t.Fatal(err)
	}
	wg.Wait()
}
