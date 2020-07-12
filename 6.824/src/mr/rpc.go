package mr

//
// RPC definitions.
//
// remember to capitalize all names.
//

import (
	"os"
	"strconv"
)

//
// example to show how to declare the arguments
// and reply for an RPC.
//

type ExampleArgs struct {
	X int
}

type ExampleReply struct {
	Y int
}

// Add your RPC definitions here.

//TaskType Comment
type TaskType int8

// Comment
const (
	TaskNone   TaskType = 0
	TaskWait   TaskType = 1
	TaskMap    TaskType = 2
	TaskReduce TaskType = 3
)

//AskTaskArgs Comment
type AskTaskArgs struct {
}

//AskTaskReply Comment
type AskTaskReply struct {
	TaskType TaskType
	NReduce  int
	NMap     int
	No       int
	File     string
}

//FinishTaskArgs Comment
type FinishTaskArgs struct {
	TaskType TaskType
	No       int
}

//FinishTaskReply Comment
type FinishTaskReply struct {
}

// Cook up a unique-ish UNIX-domain socket name
// in /var/tmp, for the master.
// Can't use the current directory since
// Athena AFS doesn't support UNIX-domain sockets.
func masterSock() string {
	s := "/var/tmp/824-mr-"
	s += strconv.Itoa(os.Getuid())
	return s
}
