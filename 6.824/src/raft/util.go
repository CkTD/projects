package raft

import (
	"math/rand"
	"time"
)

func init() {
	rand.Seed(time.Now().UnixNano())
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

func Max(x, y int) int {
	if x >= y {
		return x
	}
	return y
}

func Assert(ok bool, msg string) {
	if !ok {
		panic(msg)
	}
}
