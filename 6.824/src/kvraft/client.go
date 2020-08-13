package kvraft

import (
	"crypto/rand"
	"fmt"
	"math/big"
	"time"

	"../labrpc"
)

type Clerk struct {
	servers []*labrpc.ClientEnd
	// You will have to modify this struct.

	peersNum int
	leaderID int

	clientID  int64
	MessageID int64
}

func (ck *Clerk) Dprintf(format string, a ...interface{}) (n int, err error) {
	if Debug > 0 {
		msg := fmt.Sprintf("%v client[%d] ", time.Now().Format("15:04:05.000000"), ck.clientID) + format + "\n"
		fmt.Printf(msg, a...)
	}
	return
}

func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	x := bigx.Int64()
	return x
}

func MakeClerk(servers []*labrpc.ClientEnd) *Clerk {
	ck := new(Clerk)
	ck.servers = servers

	// You'll have to add code here.
	ck.peersNum = len(servers)
	ck.leaderID = 0
	ck.clientID = nrand()
	ck.MessageID = 0
	ck.Dprintf("new clien: %d", ck.clientID)
	return ck
}

//
// fetch the current value for a key.
// returns "" if the key does not exist.
// keeps trying forever in the face of all other errors.
//
// you can send an RPC with code like this:
// ok := ck.servers[i].Call("KVServer.Get", &args, &reply)
//
// the types of args and reply (including whether they are pointers)
// must match the declared types of the RPC handler function's
// arguments. and reply must be passed as a pointer.
//
func (ck *Clerk) Get(key string) string {
	// You will have to modify this function.
	value := ""
	ck.MessageID++
	ck.Dprintf("get[%d] %s", ck.MessageID, key)
	for {
		args := GetArgs{Key: key, ClientID: ck.clientID, MessageID: ck.MessageID}
		reply := GetReply{}

		okCh := make(chan bool)
		var ok bool
		go func() {
			ok := ck.servers[ck.leaderID].Call("KVServer.Get", &args, &reply)
			okCh <- ok
		}()

		select {
		case ok = <-okCh:
		case <-time.After(time.Second * 2):
			ok = false
		}

		if !ok || reply.Err == ErrWrongLeader { // try another
			ck.Dprintf("retry get[%d] %s", ck.MessageID, key)
			ck.leaderID = (ck.leaderID + 1) % ck.peersNum
		} else { // success
			if reply.Err == OK {
				value = reply.Value
			} else if reply.Err == ErrNoKey {
			} else {
				panic("...")
			}
			ck.Dprintf("done get[%d] %s->%s", ck.MessageID, key, reply.Value)
			break
		}
		time.Sleep(100 * time.Millisecond)
	}
	return value
}

//
// shared by Put and Append.
//
// you can send an RPC with code like this:
// ok := ck.servers[i].Call("KVServer.PutAppend", &args, &reply)
//
// the types of args and reply (including whether they are pointers)
// must match the declared types of the RPC handler function's
// arguments. and reply must be passed as a pointer.
//
func (ck *Clerk) PutAppend(key string, value string, op string) {
	// You will have to modify this function.
	ck.MessageID++
	ck.Dprintf("%s[%d] %s[%s]", op, ck.MessageID, key, value)
	for {
		args := PutAppendArgs{Key: key, Value: value, Op: op, ClientID: ck.clientID, MessageID: ck.MessageID}
		reply := PutAppendReply{}

		okCh := make(chan bool)
		var ok bool
		go func() {
			ok := ck.servers[ck.leaderID].Call("KVServer.PutAppend", &args, &reply)
			okCh <- ok
		}()

		select {
		case ok = <-okCh:
		case <-time.After(time.Second * 2):
			ok = false
		}

		if !ok || reply.Err == ErrWrongLeader {
			ck.Dprintf("retry %s[%d] %s[%s]", op, ck.MessageID, key, value)
			ck.leaderID = (ck.leaderID + 1) % ck.peersNum
		} else if reply.Err == OK { // success
			ck.Dprintf("done %s[%d] %s->%s", op, ck.MessageID, key, reply.Value)
			if reply.Err == OK {
			} else {
				panic("...")
			}
			break
		} else {
			panic("??")
		}
		time.Sleep(100 * time.Millisecond)
	}
}

func (ck *Clerk) Put(key string, value string) {
	ck.PutAppend(key, value, "Put")
}
func (ck *Clerk) Append(key string, value string) {
	ck.PutAppend(key, value, "Append")
}
