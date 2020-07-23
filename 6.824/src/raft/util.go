package raft

import (
	"fmt"
	"math/rand"
	"time"
)

func init() {
	rand.Seed(time.Now().UnixNano())
}

// Debugging
const Debug = 0

//
func DPrintf(rf *Raft, format string, a ...interface{}) (n int, err error) {
	state := ""
	switch rf.state {
	case follower:
		state = "follower"
	case candidate:
		state = "candidate"
	case leader:
		state = "leader"
	}
	msg := fmt.Sprintf("%s Peer[%d] Term[%d] State[%-9s] ", time.Now().Format("15:04:05.000000"), rf.me, rf.currentTerm, state)
	msg = msg + format + "\n"
	if Debug > 0 {
		fmt.Printf(msg, a...)
	}
	return
}

//
func RandIntRange(min, max time.Duration) time.Duration {
	d := rand.Int63n(max.Nanoseconds()-min.Nanoseconds()) + min.Nanoseconds()
	return time.Duration(d)
}

func Min(x, y int) int {
	if x <= y {
		return x
	}
	return y
}
